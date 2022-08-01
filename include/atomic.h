#ifndef __ATOMIC_H__
#define __ATOMIC_H__
#include <stdbool.h>

//the author of this API was clearly mentally challenged
static inline void     *atomic_cas_ptr(volatile void *ptr, void *old, void *new)
{
    void *local_old;
    local_old = old;
    __atomic_compare_exchange_n(&ptr, &local_old, new, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
    return local_old;
}
static inline uint8_t  atomic_cas_8(volatile uint8_t *ptr, uint8_t old, uint8_t new)
{
    uint8_t local_old;
    local_old = old;
    __atomic_compare_exchange_n(ptr, &local_old, new, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
    return local_old;
}
static inline uint16_t atomic_cas_16(volatile uint16_t *ptr, uint16_t old, uint16_t new)
{
    uint16_t local_old;
    local_old = old;
    __atomic_compare_exchange_n(ptr, &local_old, new, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
    return local_old;
}
static inline uint32_t atomic_cas_32(volatile uint32_t *ptr, uint32_t old, uint32_t new)
{
    uint32_t local_old;
    local_old = old;
    __atomic_compare_exchange_n(ptr, &local_old, new, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
    return local_old;
}
static inline uint64_t atomic_cas_64(volatile uint64_t *ptr, uint64_t old, uint64_t new)
{
    uint64_t local_old;
    local_old = old;
    __atomic_compare_exchange_n(ptr, &local_old, new, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
    return local_old;
}

#endif //__ATOMIC_H__

