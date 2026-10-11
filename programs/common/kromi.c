/* The examples' shared helpers (kromi.h).  Builds with the Kromi One SDK (ABI 2.0, revision 17). */
#include "kromi.h"

/* A program has no C library: the compiler's own copies and clears land here. */
void *memset(void *to, int value, size_t bytes);
void *memset(void *to, int value, size_t bytes)
{
    uint8_t *p = to;

    for (size_t i = 0; i < bytes; ++i) {
        p[i] = (uint8_t)value;
    }
    return to;
}

void *memcpy(void *to, const void *from, size_t bytes);
void *memcpy(void *to, const void *from, size_t bytes)
{
    uint8_t *p = to;
    const uint8_t *q = from;

    for (size_t i = 0; i < bytes; ++i) {
        p[i] = q[i];
    }
    return to;
}

/* ---- the start ---- */

/* KROM enters here: x0 the boot information, x1 its bytes, x2 the ABI it speaks.  A program checks that it speaks
 * the same, runs, and ends with its status. */
_Noreturn void _start(const krom_abi2_boot_info *boot, uint64_t boot_bytes, uint64_t abi);
_Noreturn void _start(const krom_abi2_boot_info *boot, uint64_t boot_bytes, uint64_t abi)
{
    if ((abi != KROM_ABI2_PACKED) || (boot == NULL) || (boot_bytes < sizeof(*boot))
        || (boot->magic != KROM_ABI2_BOOT_INFO_MAGIC)) {
        kromi_exit(255);
    }
    kromi_exit(kromi_main(boot));
}

/* ---- text ---- */

void kromi_say(const char *text)
{
    uint64_t bytes = 0;

    while (text[bytes] != '\0') {
        ++bytes;
    }
    kromi_write(text, bytes);
}

void kromi_say_number(const char *label, uint64_t value)
{
    char digits[24];
    uint32_t n = 0;

    do {
        digits[sizeof(digits) - 2U - n++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U);
    digits[sizeof(digits) - 1U] = '\n';
    kromi_say(label);
    kromi_write(&digits[sizeof(digits) - 1U - n], n + 1U);
}

const krom_abi2_device_info *kromi_device(const krom_abi2_boot_info *boot, uint64_t family)
{
    const uint8_t *table = (const uint8_t *)boot + boot->device_table_offset;

    for (uint32_t i = 0; i < boot->device_count; ++i) {
        const krom_abi2_device_info *device = (const void *)(table + (uint64_t)i * boot->device_entry_bytes);

        if (device->family == family) {
            return device;
        }
    }
    return NULL;
}

/* ---- memory ---- */

uint32_t kromi_memory(struct kromi_memory *memory, uint64_t bytes, uint32_t usage, bool coherent)
{
    const uint64_t coherency = coherent ? KROM_ABI2_MEMORY_COHERENT : KROM_ABI2_MEMORY_NONCOHERENT;
    krom_abi2_object_spec spec = {0};
    krom_abi2_mapping map = {0};
    uint32_t status;

    spec.byte_size = sizeof(spec);
    spec.bytes = bytes;
    spec.alignment = 4096;
    spec.required_domain_bits = KROM_ABI2_MEMORY_CPU_READ | KROM_ABI2_MEMORY_CPU_WRITE | KROM_ABI2_MEMORY_DEVICE_READ
                              | KROM_ABI2_MEMORY_DEVICE_WRITE | KROM_ABI2_MEMORY_CONTIGUOUS | coherency;
    spec.usage = usage;
    status = krom_abi2_call_object_create(&spec, sizeof(spec), &memory->object, sizeof(memory->object), NULL);
    if (status != KROM_ABI2_STATUS_OK) {
        return status;
    }
    map.byte_size = sizeof(map);
    map.flags = (uint32_t)(KROM_ABI2_MEMORY_CPU_READ | KROM_ABI2_MEMORY_CPU_WRITE | coherency);
    map.object_handle = memory->object.object_handle;
    map.mapped_bytes = bytes;
    status = krom_abi2_call_map(&map, sizeof(map), &memory->cpu, sizeof(memory->cpu), NULL);
    memory->at = (void *)(uintptr_t)memory->cpu.virtual_address;
    return status;
}

uint32_t kromi_hand_over(const struct kromi_memory *memory)
{
    krom_abi2_cache_sync sync = {0};

    sync.byte_size = sizeof(sync);
    sync.direction = (uint32_t)KROM_ABI2_SYNC_TO_DEVICE;
    sync.mapping_handle = memory->cpu.mapping_handle;
    sync.bytes = memory->object.bytes;
    sync.reset_epoch = memory->cpu.reset_epoch;
    return krom_abi2_call_cache_sync(&sync, sizeof(sync), NULL, 0, NULL);
}

/* ---- the 2D chipset ---- */

#define SLOT_BYTES 256U                 /* each command's room in the commands memory */

uint32_t kromi_2d_open(struct kromi_2d *q, const krom_abi2_boot_info *boot)
{
    const krom_abi2_device_info *chipset = kromi_device(boot, KROM_ABI2_FAMILY_KCHIPSET_2D);
    krom_abi2_lease lease = {0};
    krom_abi2_queue_create create = {0};
    uint32_t status;

    if (chipset == NULL) {
        return KROM_ABI2_STATUS_UNSUPPORTED;            /* this machine has no 2D chipset */
    }
    if (((status = kromi_memory(&q->ring, 16384, KROM_ABI2_OBJECT_USAGE_QUEUE, true)) != KROM_ABI2_STATUS_OK)
        || ((status = kromi_memory(&q->commands, 65536, KROM_ABI2_OBJECT_USAGE_GENERAL, true)) != KROM_ABI2_STATUS_OK)) {
        return status;
    }
    /* the right to use the chipset, shared with the system (KROM decides who draws when) */
    lease.byte_size = sizeof(lease);
    lease.mode = (uint32_t)KROM_ABI2_OWNERSHIP_MANAGED;
    lease.device_handle = chipset->device_handle;
    status = krom_abi2_call_lease_acquire(&lease, sizeof(lease), &q->lease, sizeof(q->lease), NULL);
    if (status != KROM_ABI2_STATUS_OK) {
        return status;
    }
    /* the queue: 64 commands in flight, its ring in the program's memory */
    create.byte_size = sizeof(create);
    create.lease_handle = q->lease.lease_handle;
    create.family = (uint16_t)KROM_ABI2_FAMILY_KCHIPSET_2D;
    create.submission_count = 64;
    create.completion_count = 64;
    create.shared_object = q->ring.object.object_handle;
    create.required_features = KROM_ABI2_QUEUE_FEATURE_COHERENT;
    status = krom_abi2_call_queue_create(&create, sizeof(create), &q->queue, sizeof(q->queue), NULL);
    q->shared = (void *)((uint8_t *)q->ring.at + q->queue.shared_offset);
    return status;
}

/* the next slot of the batch: `bytes` of command, zeroed, and its record */
static void *slot(struct kromi_2d *q, uint16_t opcode, uint32_t bytes, uint32_t access)
{
    uint8_t *at;
    krom_abi2_queue_submission *record;

    if (q->count == sizeof(q->batch) / sizeof(q->batch[0])) {
        return NULL;
    }
    at = (uint8_t *)q->commands.at + 1024U + SLOT_BYTES * q->count;
    memset(at, 0, SLOT_BYTES);
    record = &q->batch[q->count++];
    memset(record, 0, sizeof(*record));
    record->opcode = opcode;
    record->record_bytes = sizeof(*record);
    record->command_id = ++q->next_id;
    record->payload_object = q->commands.object.object_handle;
    record->payload_offset = (uint64_t)(at - (uint8_t *)q->commands.at);
    record->payload_bytes = bytes;
    record->payload_access = access;
    return at;
}

krom_abi2_k2d_command *kromi_2d_command(struct kromi_2d *q, uint16_t opcode)
{
    krom_abi2_k2d_command *command = slot(q, opcode, sizeof(*command), KROM_ABI2_PAYLOAD_DEVICE_READ);

    if (command != NULL) {
        command->byte_size = sizeof(*command);
        command->command_kind = opcode;
    }
    return command;
}

uint32_t kromi_2d_run(struct kromi_2d *q)
{
    const uint8_t *shared = (const uint8_t *)q->shared;
    krom_abi2_doorbell doorbell = {0};
    const uint64_t target = q->consumed + q->count;
    uint32_t status;

    for (uint32_t i = 0; i < q->count; ++i) {
        const uint64_t at = (q->produced + i) & (q->shared->submission_count - 1U);

        *(krom_abi2_queue_submission *)(void *)((uint8_t *)q->shared + q->shared->submission_offset
                                                 + at * q->shared->submission_stride) = q->batch[i];
    }
    q->produced += q->count;
    q->count = 0;
    __atomic_store_n(&q->shared->submission_producer, q->produced, __ATOMIC_RELEASE);
    doorbell.byte_size = sizeof(doorbell);
    doorbell.queue_handle = q->queue.queue_handle;
    doorbell.published_producer = q->produced;
    doorbell.reset_epoch = q->queue.reset_epoch;
    status = krom_abi2_call_doorbell(&doorbell, sizeof(doorbell), NULL, 0, NULL);
    if (status != KROM_ABI2_STATUS_OK) {
        return status;
    }
    /* each command's completion, in order; between them the program sleeps until KROM signals the queue */
    while (q->consumed < target) {
        const uint64_t seen = __atomic_load_n(&q->shared->notification_sequence, __ATOMIC_ACQUIRE);
        const uint64_t done = __atomic_load_n(&q->shared->completion_producer, __ATOMIC_ACQUIRE);

        while ((q->consumed < done) && (q->consumed < target)) {
            const krom_abi2_queue_completion *completion = (const void *)(shared + q->shared->completion_offset
                + (q->consumed & (q->shared->completion_count - 1U)) * q->shared->completion_stride);

            if (completion->status != KROM_ABI2_QUEUE_STATUS_OK) {
                return completion->status;
            }
            ++q->consumed;
        }
        __atomic_store_n(&q->shared->completion_consumer, q->consumed, __ATOMIC_RELEASE);
        if (q->consumed < target) {
            krom_abi2_wait wait = {0};

            wait.byte_size = sizeof(wait);
            wait.notification_id = q->queue.notification_id;
            wait.observed_sequence = seen;
            wait.deadline_ns = UINT64_MAX;
            (void)krom_abi2_call_wait(&wait, sizeof(wait), NULL, 0, NULL);
        }
    }
    return KROM_ABI2_STATUS_OK;
}

uint32_t kromi_2d_picture(struct kromi_2d *q, struct kromi_memory *pixels, uint32_t width, uint32_t height,
                          uint16_t format, uint64_t *picture)
{
    krom_abi2_mapping map = {0}, mapped = {0};
    krom_abi2_k2d_surface *surface;
    uint32_t status;

    status = kromi_memory(pixels, (uint64_t)width * height * 4U, KROM_ABI2_OBJECT_USAGE_SURFACE, false);
    if (status != KROM_ABI2_STATUS_OK) {
        return status;
    }
    /* the chipset's view of the same memory */
    map.byte_size = sizeof(map);
    map.flags = (uint32_t)(KROM_ABI2_MEMORY_DEVICE_READ | KROM_ABI2_MEMORY_DEVICE_WRITE | KROM_ABI2_MEMORY_NONCOHERENT);
    map.object_handle = pixels->object.object_handle;
    map.mapped_bytes = pixels->object.bytes;
    map.device_handle = q->lease.device_handle;
    status = krom_abi2_call_map(&map, sizeof(map), &mapped, sizeof(mapped), NULL);
    if (status != KROM_ABI2_STATUS_OK) {
        return status;
    }
    /* KROM checks the picture's size and format once; commands then name it by its handle */
    surface = slot(q, (uint16_t)KROM_ABI2_K2D_OPCODE_VALIDATE_SURFACE, sizeof(*surface),
                   KROM_ABI2_PAYLOAD_DEVICE_READ | KROM_ABI2_PAYLOAD_DEVICE_WRITE);
    if (surface == NULL) {
        return KROM_ABI2_STATUS_LIMIT_EXCEEDED;
    }
    surface->byte_size = sizeof(*surface);
    surface->object_handle = pixels->object.object_handle;
    surface->object_bytes = pixels->object.bytes;
    surface->width = width;
    surface->height = height;
    surface->stride_bytes = width * 4U;
    surface->format = format;
    surface->scanout_domain = (uint16_t)KROM_ABI2_K2D_SCANOUT_CPU_LINEAR;
    status = kromi_2d_run(q);
    *picture = surface->surface_handle;
    return ((status == KROM_ABI2_STATUS_OK) && (*picture == 0U)) ? KROM_ABI2_STATUS_INVALID_ARGUMENT : status;
}
