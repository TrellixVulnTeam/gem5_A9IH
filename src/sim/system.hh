/*
 * Copyright (c) 2012, 2014, 2018 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * Copyright (c) 2011 Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SYSTEM_HH__
#define __SYSTEM_HH__

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "arch/isa_traits.hh"
#include "base/loader/memory_image.hh"
#include "base/loader/symtab.hh"
#include "base/statistics.hh"
#include "config/the_isa.hh"
#include "cpu/pc_event.hh"
#include "enums/MemoryMode.hh"
#include "mem/mem_master.hh"
#include "mem/physical.hh"
#include "mem/port.hh"
#include "mem/port_proxy.hh"
#include "params/System.hh"
#include "sim/futex_map.hh"
#include "sim/redirect_path.hh"
#include "sim/se_signal.hh"
#include "sim/sim_object.hh"
#include "sim/workload.hh"
#include "aladdin/gem5/aladdin_sys_connection.h"

class BaseRemoteGDB;
class KvmVM;
class ThreadContext;
class Gem5Datapath;

class System : public SimObject, public PCEventScope
{
  private:

    /**
     * Private class for the system port which is only used as a
     * master for debug access and for non-structural entities that do
     * not have a port of their own.
     */
    class SystemPort : public MasterPort
    {
      public:

        /**
         * Create a system port with a name and an owner.
         */
        SystemPort(const std::string &_name, SimObject *_owner)
            : MasterPort(_name, _owner)
        { }
        bool recvTimingResp(PacketPtr pkt) override
        { panic("SystemPort does not receive timing!\n"); return false; }
        void recvReqRetry() override
        { panic("SystemPort does not expect retry!\n"); }
    };

    std::list<PCEvent *> liveEvents;
    SystemPort _systemPort;

  public:

    void init() override;
    void startup() override;

    /**
     * Get a reference to the system port that can be used by
     * non-structural simulation objects like processes or threads, or
     * external entities like loaders and debuggers, etc, to access
     * the memory system.
     *
     * @return a reference to the system port we own
     */
    MasterPort& getSystemPort() { return _systemPort; }

    /**
     * Additional function to return the Port of a memory object.
     */
    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    /** @{ */
    /**
     * Is the system in atomic mode?
     *
     * There are currently two different atomic memory modes:
     * 'atomic', which supports caches; and 'atomic_noncaching', which
     * bypasses caches. The latter is used by hardware virtualized
     * CPUs. SimObjects are expected to use Port::sendAtomic() and
     * Port::recvAtomic() when accessing memory in this mode.
     */
    bool isAtomicMode() const {
        return memoryMode == Enums::atomic ||
            memoryMode == Enums::atomic_noncaching;
    }

    /**
     * Is the system in timing mode?
     *
     * SimObjects are expected to use Port::sendTiming() and
     * Port::recvTiming() when accessing memory in this mode.
     */
    bool isTimingMode() const {
        return memoryMode == Enums::timing;
    }

    /**
     * Should caches be bypassed?
     *
     * Some CPUs need to bypass caches to allow direct memory
     * accesses, which is required for hardware virtualization.
     */
    bool bypassCaches() const {
        return memoryMode == Enums::atomic_noncaching;
    }
    /** @} */

    /** @{ */
    /**
     * Get the memory mode of the system.
     *
     * \warn This should only be used by the Python world. The C++
     * world should use one of the query functions above
     * (isAtomicMode(), isTimingMode(), bypassCaches()).
     */
    Enums::MemoryMode getMemoryMode() const { return memoryMode; }

    /**
     * Change the memory mode of the system.
     *
     * \warn This should only be called by the Python!
     *
     * @param mode Mode to change to (atomic/timing/...)
     */
    void setMemoryMode(Enums::MemoryMode mode);
    /** @} */

    /**
     * Get the cache line size of the system.
     */
    unsigned int cacheLineSize() const { return _cacheLineSize; }

    std::vector<ThreadContext *> threadContexts;
    ThreadContext *findFreeContext();

    ThreadContext *
    getThreadContext(ContextID tid) const
    {
        return threadContexts[tid];
    }

    const bool multiThread;

    using SimObject::schedule;

    bool schedule(PCEvent *event) override;
    bool remove(PCEvent *event) override;

    unsigned numContexts() const { return threadContexts.size(); }

    /* Maps an accelerator id to a Gem5Datapath object. The id can be an IOCTL
     * request code. When gem5 intercepts the ioctl syscall, it will schedule
     * the accelerator given by the request code for execution. The size of the
     * map is the number of executing accelerators.
     */
    std::map<int, Gem5Datapath*> accelerators;

    /* Returns the number of accelerators that are currently registered and
     * running in the system.
     */
    int numRunningAccelerators() { return accelerators.size(); }

    /* Check if the accelerator exists. */
    void checkAcceleratorExists(int id, const std::string& funcName);

    /* Registers the datapath pointer and list of dependencies with the system.
     * If the accelerator already exists, the simulation ends with a fatal
     * message.
     */
    void registerAccelerator(int id, Gem5Datapath* accelerator);

    /* Marks an accelerator as finished by erasing it from the registered list.
     */
    void deregisterAccelerator(int id);

    /* Activates an accelerator with the provided parameters. */
    void activateAccelerator(unsigned accel_id,
                             Addr finish_flag,
                             std::unique_ptr<uint8_t[]>
                                 accel_params,
                             int context_id,
                             int thread_id);

    /* Add an address tranlation into the datapath TLB for the specified array.
     */
    void insertAddressTranslationMapping(int id,
                                         Addr sim_vaddr,
                                         Addr sim_paddr);

    /* Add an mapping between array names to the simulated virtual addresses. */
    void insertArrayLabelMapping(int id, std::string array_label,
                                 Addr sim_vaddr, size_t size);

    /* Set memory access type for an array. */
    void setArrayMemoryType(int id, std::string array_label,
                            MemoryType mem_type);

    /* Get the base trace address of of the array for the specified accelerator.
     */
    Addr getArrayBaseAddress(int id, const char* array_name);

    /** Return number of running (non-halted) thread contexts in
     * system.  These threads could be Active or Suspended. */
    int numRunningContexts();

    Addr pagePtr;

    uint64_t init_param;

    /** Port to physical memory used for writing object files into ram at
     * boot.*/
    PortProxy physProxy;

    /** OS kernel */
    Workload *workload = nullptr;

  public:
    /**
     * Get a pointer to the Kernel Virtual Machine (KVM) SimObject,
     * if present.
     */
    KvmVM* getKvmVM() {
        return kvmVM;
    }

    /** Verify gem5 configuration will support KVM emulation */
    bool validKvmEnvironment() const;

    /** Get a pointer to access the physical memory of the system */
    PhysicalMemory& getPhysMem() { return physmem; }

    /** Amount of physical memory that is still free */
    Addr freeMemSize() const;

    /** Amount of physical memory that exists */
    Addr memSize() const;

    /**
     * Check if a physical address is within a range of a memory that
     * is part of the global address map.
     *
     * @param addr A physical address
     * @return Whether the address corresponds to a memory
     */
    bool isMemAddr(Addr addr) const;

    /**
     * Get the architecture.
     */
    Arch getArch() const { return Arch::TheISA; }

    /**
     * Get the guest byte order.
     */
    ByteOrder
    getGuestByteOrder() const
    {
#if THE_ISA != NULL_ISA
        return TheISA::GuestByteOrder;
#else
        panic("The NULL ISA has no endianness.");
#endif
    }

     /**
     * Get the page bytes for the ISA.
     */
    Addr getPageBytes() const { return TheISA::PageBytes; }

    /**
     * Get the number of bits worth of in-page address for the ISA.
     */
    Addr getPageShift() const { return TheISA::PageShift; }

    /**
     * The thermal model used for this system (if any).
     */
    ThermalModel * getThermalModel() const { return thermalModel; }

  protected:

    KvmVM *const kvmVM;

    PhysicalMemory physmem;

    Enums::MemoryMode memoryMode;

    const unsigned int _cacheLineSize;

    uint64_t workItemsBegin;
    uint64_t workItemsEnd;
    uint32_t numWorkIds;
    std::vector<bool> activeCpus;

    /** This array is a per-system list of all devices capable of issuing a
     * memory system request and an associated string for each master id.
     * It's used to uniquely id any master in the system by name for things
     * like cache statistics.
     */
    std::vector<MasterInfo> masters;

    ThermalModel * thermalModel;

  protected:
    /**
     * Strips off the system name from a master name
     */
    std::string stripSystemName(const std::string& master_name) const;

  public:

    /**
     * Request an id used to create a request object in the system. All objects
     * that intend to issues requests into the memory system must request an id
     * in the init() phase of startup. All master ids must be fixed by the
     * regStats() phase that immediately precedes it. This allows objects in
     * the memory system to understand how many masters may exist and
     * appropriately name the bins of their per-master stats before the stats
     * are finalized.
     *
     * Registers a MasterID:
     * This method takes two parameters, one of which is optional.
     * The first one is the master object, and it is compulsory; in case
     * a object has multiple (sub)masters, a second parameter must be
     * provided and it contains the name of the submaster. The method will
     * create a master's name by concatenating the SimObject name with the
     * eventual submaster string, separated by a dot.
     *
     * As an example:
     * For a cpu having two masters: a data master and an instruction master,
     * the method must be called twice:
     *
     * instMasterId = getMasterId(cpu, "inst");
     * dataMasterId = getMasterId(cpu, "data");
     *
     * and the masters' names will be:
     * - "cpu.inst"
     * - "cpu.data"
     *
     * @param master SimObject related to the master
     * @param submaster String containing the submaster's name
     * @return the master's ID.
     */
    MasterID getMasterId(const SimObject* master,
                         std::string submaster = std::string());

    /**
     * Registers a GLOBAL MasterID, which is a MasterID not related
     * to any particular SimObject; since no SimObject is passed,
     * the master gets registered by providing the full master name.
     *
     * @param masterName full name of the master
     * @return the master's ID.
     */
    MasterID getGlobalMasterId(const std::string& master_name);

    /**
     * Get the name of an object for a given request id.
     */
    std::string getMasterName(MasterID master_id);

    /**
     * Looks up the MasterID for a given SimObject
     * returns an invalid MasterID (invldMasterId) if not found.
     */
    MasterID lookupMasterId(const SimObject* obj) const;

    /**
     * Looks up the MasterID for a given object name string
     * returns an invalid MasterID (invldMasterId) if not found.
     */
    MasterID lookupMasterId(const std::string& name) const;

    /** Get the number of masters registered in the system */
    MasterID maxMasters() { return masters.size(); }

  protected:
    /** helper function for getMasterId */
    MasterID _getMasterId(const SimObject* master,
                          const std::string& master_name);

    /**
     * Helper function for constructing the full (sub)master name
     * by providing the root master and the relative submaster name.
     */
    std::string leafMasterName(const SimObject* master,
                               const std::string& submaster);

  public:

    void regStats() override;
    /**
     * Called by pseudo_inst to track the number of work items started by this
     * system.
     */
    uint64_t
    incWorkItemsBegin()
    {
        return ++workItemsBegin;
    }

    /**
     * Called by pseudo_inst to track the number of work items completed by
     * this system.
     */
    uint64_t
    incWorkItemsEnd()
    {
        return ++workItemsEnd;
    }

    /**
     * Called by pseudo_inst to mark the cpus actively executing work items.
     * Returns the total number of cpus that have executed work item begin or
     * ends.
     */
    int
    markWorkItem(int index)
    {
        int count = 0;
        assert(index < activeCpus.size());
        activeCpus[index] = true;
        for (std::vector<bool>::iterator i = activeCpus.begin();
             i < activeCpus.end(); i++) {
            if (*i) count++;
        }
        return count;
    }

    inline void workItemBegin(uint32_t tid, uint32_t workid)
    {
        std::pair<uint32_t,uint32_t> p(tid, workid);
        lastWorkItemStarted[p] = curTick();
    }

    void workItemEnd(uint32_t tid, uint32_t workid);

  public:
    std::vector<BaseRemoteGDB *> remoteGDB;
    bool breakpoint();

  public:
    typedef SystemParams Params;

  protected:
    Params *_params;

    /**
     * Range for memory-mapped m5 pseudo ops. The range will be
     * invalid/empty if disabled.
     */
    const AddrRange _m5opRange;

  public:
    System(Params *p);
    ~System();

    const Params *params() const { return (const Params *)_params; }

    /**
     * Range used by memory-mapped m5 pseudo-ops if enabled. Returns
     * an invalid/empty range if disabled.
     */
    const AddrRange &m5opRange() const { return _m5opRange; }

  public:

    /// Allocate npages contiguous unused physical pages
    /// @return Starting address of first page
    Addr allocPhysPages(int npages);

    ContextID registerThreadContext(ThreadContext *tc,
                                    ContextID assigned = InvalidContextID);
    void replaceThreadContext(ThreadContext *tc, ContextID context_id);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    void drainResume() override;

  public:
    Counter totalNumInsts;
    std::map<std::pair<uint32_t,uint32_t>, Tick>  lastWorkItemStarted;
    std::map<uint32_t, Stats::Histogram*> workItemStats;

    ////////////////////////////////////////////
    //
    // STATIC GLOBAL SYSTEM LIST
    //
    ////////////////////////////////////////////

    static std::vector<System *> systemList;
    static int numSystemsRunning;

    static void printSystems();

    FutexMap futexMap;

    static const int maxPID = 32768;

    /** Process set to track which PIDs have already been allocated */
    std::set<int> PIDs;

    // By convention, all signals are owned by the receiving process. The
    // receiver will delete the signal upon reception.
    std::list<BasicSignal> signalList;

    // Used by syscall-emulation mode. This member contains paths which need
    // to be redirected to the faux-filesystem (a duplicate filesystem
    // intended to replace certain files on the host filesystem).
    std::vector<RedirectPath*> redirectPaths;
};

void printSystems();

#endif // __SYSTEM_HH__
