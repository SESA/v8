//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <malloc.h>
#include <sys/time.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "src/base/macros.h"
#include "src/base/atomicops.h"
#include "src/base/platform/condition-variable.h"
#include "src/base/platform/mutex.h"
#include "src/base/platform/platform.h"
#include "src/base/platform/semaphore.h"
#include "src/base/platform/time.h"
#include "src/libsampler/sampler.h"
#include "src/interpreter/bytecode-peephole-table.h"

#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/native/Clock.h>
#include <ebbrt/native/Fls.h>
#include <ebbrt/native/PageAllocator.h>
#include <ebbrt/native/VMem.h>
#include <ebbrt/native/VMemAllocator.h>

#undef UNIMPLEMENTED
#define UNIMPLEMENTED() \
  fprintf(stderr,"[ FATAL ] UNIMPLEMENTED FUNCTION: %s\n", __PRETTY_FUNCTION__); \
  abort();

#undef FATAL_ERROR
#define FATAL_ERROR() \
  fprintf(stderr,"[ FATAL ] FAILURE: %s\n", __PRETTY_FUNCTION__); \
  abort();

#define WARN_UNIMPLEMENTED() \
  ebbrt::kprintf("[ WARNING ] UNIMPLEMENTED FUNCTION: %s\n", __PRETTY_FUNCTION__); 

namespace {
std::mutex limit_mut;
std::mutex key_map_mut;
std::unordered_map<int, __gthread_key_t> key_map
    __attribute__((init_priority(101)));
}

v8::base::Semaphore::Semaphore(int count){
    WARN_UNIMPLEMENTED();
}

v8::base::ConditionVariable::NativeHandle::~NativeHandle() {
  WARN_UNIMPLEMENTED();
}

void v8::sampler::Sampler::DoSample(){
  UNIMPLEMENTED();
}

bool v8::base::Semaphore::WaitFor(v8::base::TimeDelta const&){
  WARN_UNIMPLEMENTED();
  return true;
}

v8::base::Time v8::base::Time::NowFromSystemTime(){
  UNIMPLEMENTED();
}

v8::base::Semaphore::~Semaphore(){}

void v8::base::Semaphore::Wait(){
  WARN_UNIMPLEMENTED();
}

void v8::base::Semaphore::Signal(){
  WARN_UNIMPLEMENTED();
}

void v8::base::ConditionVariable::Wait(v8::base::Mutex*){
  UNIMPLEMENTED();
}

void v8::base::ConditionVariable::NotifyOne(){
  UNIMPLEMENTED();
}

bool v8::base::ConditionVariable::WaitFor(v8::base::Mutex*, v8::base::TimeDelta const&){
  UNIMPLEMENTED();
  return false;
}

v8::base::ConditionVariable::ConditionVariable(){
  WARN_UNIMPLEMENTED();
}

v8::base::ConditionVariable::~ConditionVariable(){
  WARN_UNIMPLEMENTED();
}

void v8::base::OS::Initialize(int64_t random_seed,
                         bool hard_abort,
                         const char* const gc_fake_mmap){
  WARN_UNIMPLEMENTED();
  return;
}

void* v8::base::OS::AllocateGuarded(const size_t requested){
  UNIMPLEMENTED();
  return nullptr;
}

void v8::base::OS::Unprotect(void* address, const size_t size){
  UNIMPLEMENTED();
  return;
}


v8::base::TimezoneCache* v8::base::OS::CreateTimezoneCache(){
  return NULL;
};

void v8::base::OS::ClearTimezoneCache(v8::base::TimezoneCache* cache){
  DCHECK(cache == NULL);
}

void v8::base::OS::DisposeTimezoneCache(v8::base::TimezoneCache* cache){
  DCHECK(cache == NULL);
}

int v8::base::OS::GetUserTime(uint32_t* secs, uint32_t* usecs) {
  UNIMPLEMENTED();
  return 0;
}

double v8::base::OS::LocalTimeOffset(v8::base::TimezoneCache* cache){
  UNIMPLEMENTED();
  return 0;
}

double v8::base::OS::TimeCurrentMillis() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) return 0.0;
  return (static_cast<double>(tv.tv_sec) * 1000) +
         (static_cast<double>(tv.tv_usec) / 1000);
}

const char* v8::base::OS::LocalTimezone(double time, TimezoneCache* cache) {
  UNIMPLEMENTED();
  return nullptr;
}

double v8::base::OS::DaylightSavingsOffset(double time, TimezoneCache* cache ) {
  UNIMPLEMENTED();
  return 0;
}

int v8::base::OS::GetLastError() {
  UNIMPLEMENTED();
  return 0;
}

FILE* v8::base::OS::FOpen(const char* path, const char* mode) {
  UNIMPLEMENTED();
  return nullptr;
}

bool v8::base::OS::Remove(const char* path) {
  UNIMPLEMENTED();
  return false;
}

FILE* v8::base::OS::OpenTemporaryFile() {
  UNIMPLEMENTED();
  return nullptr;
}

const char* const v8::base::OS::LogFileOpenMode = "w";

void v8::base::OS::Print(const char* format, ...) { 
  va_list args;
  va_start(args, format);
  VPrint(format, args);
  va_end(args);
}

void v8::base::OS::VPrint(const char* format, va_list args) {
  vprintf(format, args);
}

void v8::base::OS::FPrint(FILE* out, const char* format, ...) {
  va_list args;
  va_start(args, format);
  VFPrint(out, format, args);
  va_end(args);
}

void v8::base::OS::VFPrint(FILE* out, const char* format, va_list args) {
  vfprintf(out, format, args);
}

void v8::base::OS::PrintError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

void v8::base::OS::VPrintError(const char* format, va_list args) {
  vfprintf(stderr, format, args);
}

namespace {
  uintptr_t lowest_ever_allocated = UINTPTR_MAX;
  uintptr_t highest_ever_allocated = 0;

  void UpdateAllocatedSpaceLimits(void* address, int size) {
    std::lock_guard<std::mutex> lock(limit_mut);
    auto addr = reinterpret_cast<uintptr_t>(address);
    lowest_ever_allocated = std::min(lowest_ever_allocated, addr);
    highest_ever_allocated = std::max(highest_ever_allocated, addr + size);
  }
}

void* v8::base::OS::Allocate(const size_t requested, size_t* allocated,
                                 bool is_executable) {
  const size_t msize = RoundUp(requested, AllocateAlignment());
  auto mbase = malloc(msize);

  *allocated = msize;
  UpdateAllocatedSpaceLimits(mbase, msize);
  return mbase;
}

void v8::base::OS::Free(void* address, const size_t size) {
  free(address);
}

intptr_t v8::base::OS::CommitPageSize() { return ebbrt::pmem::kPageSize; }

void v8::base::OS::ProtectCode(void* address, const size_t size) {
  WARN_UNIMPLEMENTED();
}

void v8::base::OS::Guard(void* address, const size_t size) {
  UNIMPLEMENTED();
}

void* v8::base::OS::GetRandomMmapAddr() {
  UNIMPLEMENTED();
  return nullptr;
}

size_t v8::base::OS::AllocateAlignment() {
  return 8; 
}

void v8::base::OS::Sleep(TimeDelta interval) { UNIMPLEMENTED(); }

void v8::base::OS::Abort() { abort(); }

void v8::base::OS::DebugBreak() { UNIMPLEMENTED(); }

v8::base::OS::MemoryMappedFile* v8::base::OS::MemoryMappedFile::open(
    const char* name) {
  UNIMPLEMENTED();
  return nullptr;
}

v8::base::OS::MemoryMappedFile* v8::base::OS::MemoryMappedFile::create(const char* c, size_t t, void* p){
  UNIMPLEMENTED();
  return nullptr;
}

int v8::base::OS::SNPrintF(char* str, int length,
                                          const char* format, ...){
  va_list args;
  va_start(args, format);
  auto result = VSNPrintF(str, length, format, args);
  va_end(args);
  return result;
}

int v8::base::OS::VSNPrintF(char* str, int length,
                                           const char* format, va_list args) {
 vsnprintf(str, length, format, args);
 return 0;
}

char* v8::base::OS::StrChr(char* str, int c) {
  UNIMPLEMENTED();
  return nullptr;
}

void v8::base::OS::StrNCpy(char* dest, int length, const char* src, size_t n) {
  UNIMPLEMENTED();
}

std::vector<v8::base::OS::SharedLibraryAddress> v8::base::OS::GetSharedLibraryAddresses(){
  std::vector<v8::base::OS::SharedLibraryAddress> ret;
  return ret;
}

void v8::base::OS::SignalCodeMovingGC() { UNIMPLEMENTED(); }

char v8::base::OS::DirectorySeparator() {
  return '/';
}

bool v8::base::OS::isDirectorySeparator(const char ch) {
  return ch == DirectorySeparator();
}

bool v8::base::OS::ArmUsingHardFloat() {
  UNIMPLEMENTED();
  return false;
}

int v8::base::OS::ActivationFrameAlignment() { return 16; }

int v8::base::OS::GetCurrentProcessId() {
  return (size_t)ebbrt::Cpu::GetMine();;
}

v8::base::VirtualMemory::VirtualMemory() : address_{nullptr}, size_(0) {}

v8::base::VirtualMemory::VirtualMemory(size_t size) 
  : address_(ReserveRegion(size)), size_(size) {
    size_ = size;
    address_ = malloc(size);
    if(!address_){
      printf("PANIC! Tried to allocate memory size %lu \n", size);
      FATAL_ERROR();
    }
  }

v8::base::VirtualMemory::VirtualMemory(size_t size, size_t alignment) 
    : address_(NULL), size_(0) {
    size_ = size;
    address_ = memalign(alignment, size);
    if(!address_){
      printf("PANIC! Tried to allocate memory size %lu \n", size);
      FATAL_ERROR();
    }
}

v8::base::VirtualMemory::~VirtualMemory() {
  if (address_ != nullptr) {
    WARN_UNIMPLEMENTED();
  }
}

bool v8::base::VirtualMemory::ReleasePartialRegion(void* base, size_t size, void* free_start,
                                   size_t free_size){
  UNIMPLEMENTED();
  return false;
}

bool v8::base::VirtualMemory::HasLazyCommits(){ 
  UNIMPLEMENTED();
  return false;
}

bool v8::base::VirtualMemory::IsReserved() { return address_ != nullptr; }

void v8::base::VirtualMemory::Reset() { address_ = nullptr; }

bool v8::base::VirtualMemory::Commit(void* address, size_t size,
                                         bool is_executable) {
  return CommitRegion(address, size, is_executable);
}

bool v8::base::VirtualMemory::Uncommit(void* address, size_t size) {
  return UncommitRegion(address, size);
}

bool v8::base::VirtualMemory::Guard(void* address) {
  WARN_UNIMPLEMENTED();
  return true;
}

void* v8::base::VirtualMemory::ReserveRegion(size_t size) {
  auto address = malloc(size);
    if(!address){
      FATAL_ERROR();
    }
    return address;
}

bool v8::base::VirtualMemory::CommitRegion(void* base, size_t size,
                                               bool is_executable) {
  WARN_UNIMPLEMENTED();
  return true;
}

bool v8::base::VirtualMemory::UncommitRegion(void* base, size_t size) {
  WARN_UNIMPLEMENTED();
  return true;
}

bool v8::base::VirtualMemory::ReleaseRegion(void* base, size_t size) {
  WARN_UNIMPLEMENTED();
  return false;
}

v8::base::Thread::Thread(const Options& options) {
  ebbrt::kprintf("v8 thread create: %s stack_len: %llu\n", options.name(),
                 options.stack_size());
  set_name(options.name());
}

v8::base::Thread::~Thread() { WARN_UNIMPLEMENTED(); }

void v8::base::Thread::Start() {  WARN_UNIMPLEMENTED(); }

int v8::base::OS::GetCurrentThreadId(){
  WARN_UNIMPLEMENTED();
  return 1;
}

void v8::base::Thread::set_name(const char* name) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}

void v8::base::Thread::Join() { WARN_UNIMPLEMENTED(); }

v8::base::Thread::LocalStorageKey
v8::base::Thread::CreateThreadLocalKey() {
  std::lock_guard<std::mutex> lock(key_map_mut);
  static_assert(sizeof(int) == sizeof(LocalStorageKey), "Size mismatch");
  __gthread_key_t key;
  ebbrt_gthread_key_create(&key, nullptr);
  LocalStorageKey lskey =
      static_cast<LocalStorageKey>(std::hash<__gthread_key_t>()(key));
  auto p = key_map.emplace(lskey, key);
  ebbrt::kbugon(!p.second, "Key hash collision!\n");
  return lskey;
}

void v8::base::Thread::DeleteThreadLocalKey(LocalStorageKey key) {
  UNIMPLEMENTED();
}

void *v8::base::Thread::GetThreadLocal(LocalStorageKey key) {
  std::lock_guard<std::mutex> lock(key_map_mut);
  auto it = key_map.find(key);
  ebbrt::kbugon(it == key_map.end(), "Could not find key in map\n");
  return ebbrt_gthread_getspecific(it->second);
}

void v8::base::Thread::SetThreadLocal(LocalStorageKey key, void *value) {
  std::lock_guard<std::mutex> lock(key_map_mut);
  auto it = key_map.find(key);
  ebbrt::kbugon(it == key_map.end(), "Could not find key in map\n");
  ebbrt_gthread_setspecific(it->second, value);
}

