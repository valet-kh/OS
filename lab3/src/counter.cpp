#include "counter.hpp"

SharedCounter::SharedCounter(const char* name) {
    shmem = new cplib::SharedMem<int>(name, true);
}

SharedCounter::~SharedCounter() {
    delete shmem;
}

bool SharedCounter::is_valid() const {
    return shmem && shmem->IsValid();
}

int SharedCounter::get() {
    if (!is_valid()) return 0;
    shmem->Lock();
    int value = *shmem->Data();
    shmem->Unlock();
    return value;
}

void SharedCounter::set(int value) {
    if (!is_valid()) return;
    shmem->Lock();
    *shmem->Data() = value;
    shmem->Unlock();
}

void SharedCounter::increment() {
    if (!is_valid()) return;
    shmem->Lock();
    int* val = shmem->Data();
    (*val)++;
    shmem->Unlock();
}

void SharedCounter::add(int delta) {
    if (!is_valid()) return;
    shmem->Lock();
    int* val = shmem->Data();
    *val += delta;
    shmem->Unlock();
}

void SharedCounter::multiply(int factor) {
    if (!is_valid()) return;
    shmem->Lock();
    int* val = shmem->Data();
    *val *= factor;
    shmem->Unlock();
}

void SharedCounter::divide(int divisor) {
    if (divisor == 0 || !is_valid()) return;
    shmem->Lock();
    int* val = shmem->Data();
    *val /= divisor;
    shmem->Unlock();
}

bool SharedCounter::try_become_active(int pid) {
    if (!is_valid()) return false;
    return shmem->TryBecomeActive(pid);
}

void SharedCounter::release_active(int pid) {
    if (!is_valid()) return;
    shmem->ReleaseActive(pid);
}

int SharedCounter::get_active_pid() {
    if (!is_valid()) return 0;
    return shmem->GetActivePid();
}