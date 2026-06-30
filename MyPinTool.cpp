#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>
#include <string>

using std::endl;
using std::ofstream;

/* ===================== GLOBALS ===================== */

ofstream outFile;
PIN_LOCK pinLock;

/* ---------- malloc tracking ---------- */

std::map<THREADID, ADDRINT> malloc_size_map;

/* ---------- calloc tracking ---------- */

struct CallocData
{
    ADDRINT nmemb;
    ADDRINT size;
};

std::map<THREADID, CallocData> calloc_map;

/* ---------- realloc tracking ---------- */

struct ReallocData
{
    ADDRINT old_ptr;
    ADDRINT new_size;
};

std::map<THREADID, ReallocData> realloc_map;

/* ---------- active allocations ---------- */

std::map<ADDRINT, ADDRINT> active_allocs;

/* ---------- freed pointers ---------- */

std::map<ADDRINT, bool> freed_ptrs;

/* ===================== COUNTERS ===================== */

UINT64 malloc_count = 0;
UINT64 calloc_count = 0;
UINT64 realloc_count = 0;
UINT64 free_count = 0;

UINT64 invalid_free_count = 0;
UINT64 double_free_count = 0;

/* ===================== MALLOC ===================== */

/* BEFORE malloc → store size */

VOID BeforeMalloc(THREADID tid, ADDRINT size)
{
    malloc_size_map[tid] = size;
}

/* AFTER malloc → get pointer */

VOID AfterMalloc(THREADID tid, ADDRINT ret)
{
    PIN_GetLock(&pinLock, tid + 1);

    if (ret != 0 && malloc_size_map.count(tid))
    {
        ADDRINT size = malloc_size_map[tid];

        active_allocs[ret] = size;

        freed_ptrs.erase(ret);

        malloc_count++;

        outFile << "[MALLOC] Address: 0x"
                << std::hex << ret
                << std::dec
                << " | Size: "
                << size
                << " bytes"
                << endl;

        malloc_size_map.erase(tid);
    }

    PIN_ReleaseLock(&pinLock);
}

/* ===================== CALLOC ===================== */

/* BEFORE calloc → store nmemb and size */

VOID BeforeCalloc(THREADID tid,
                  ADDRINT nmemb,
                  ADDRINT size)
{
    calloc_map[tid].nmemb = nmemb;
    calloc_map[tid].size = size;
}

/* AFTER calloc → get pointer */

VOID AfterCalloc(THREADID tid,ADDRINT ret)
{
    PIN_GetLock(&pinLock, tid + 1);

    if (ret != 0 && calloc_map.count(tid))
    {
        ADDRINT total_size =
            calloc_map[tid].nmemb *
            calloc_map[tid].size;

        active_allocs[ret] = total_size;

        freed_ptrs.erase(ret);

        calloc_count++;

        outFile << "[CALLOC] Address: 0x"
                << std::hex << ret
                << std::dec
                << " | Size: "
                << total_size
                << " bytes"
                << endl;

        calloc_map.erase(tid);
    }

    PIN_ReleaseLock(&pinLock);
}

/* ===================== REALLOC ===================== */

/* BEFORE realloc */

VOID BeforeRealloc(THREADID tid,
                   ADDRINT old_ptr,
                   ADDRINT new_size)
{
    realloc_map[tid].old_ptr = old_ptr;
    realloc_map[tid].new_size = new_size;
}

/* AFTER realloc */

VOID AfterRealloc(THREADID tid,
                  ADDRINT ret)
{
    PIN_GetLock(&pinLock, tid + 1);

    if (realloc_map.count(tid))
    {
        ADDRINT old_ptr = realloc_map[tid].old_ptr;
        ADDRINT new_size = realloc_map[tid].new_size;

        /* remove old allocation */

        if (active_allocs.count(old_ptr))
        {
            active_allocs.erase(old_ptr);
        }

        /* add new allocation */

        if (ret != 0)
        {
            active_allocs[ret] = new_size;

            freed_ptrs.erase(ret);

            realloc_count++;

            outFile << "[REALLOC] Old: 0x"
                    << std::hex << old_ptr
                    << " -> New: 0x"
                    << ret
                    << std::dec
                    << " | Size: "
                    << new_size
                    << " bytes"
                    << endl;
        }

        realloc_map.erase(tid);
    }

    PIN_ReleaseLock(&pinLock);
}

/* ===================== FREE ===================== */

VOID BeforeFree(ADDRINT ptr)
{
    PIN_GetLock(&pinLock, 1);

    if (ptr == 0)
    {
        PIN_ReleaseLock(&pinLock);
        return;
    }

    if (active_allocs.count(ptr))
    {
        /* valid free */

        free_count++;

        outFile << "[FREE] Address: 0x"
                << std::hex << ptr
                << endl;

        active_allocs.erase(ptr);

        freed_ptrs[ptr] = true;
    }
    else if (freed_ptrs.count(ptr))
    {
        /* double free */

        double_free_count++;

        outFile << "[DOUBLE FREE DETECTED] Address: 0x"
                << std::hex << ptr
                << endl;
    }
    else
    {
        /* invalid free */

        invalid_free_count++;

        outFile << "[INVALID FREE DETECTED] Address: 0x"
                << std::hex << ptr
                << endl;
    }

    PIN_ReleaseLock(&pinLock);
}

/* ===================== IMAGE LOAD ===================== */

VOID ImageLoad(IMG img, VOID *v)
{
    for (SEC sec = IMG_SecHead(img);
         SEC_Valid(sec);
         sec = SEC_Next(sec))
    {
        for (RTN rtn = SEC_RtnHead(sec);
             RTN_Valid(rtn);
             rtn = RTN_Next(rtn))
        {
            std::string name = RTN_Name(rtn);

            /* ================= malloc ================= */

            if (name.find("malloc") != std::string::npos)
            {
                RTN_Open(rtn);

                RTN_InsertCall(
                    rtn,
                    IPOINT_BEFORE,
                    (AFUNPTR)BeforeMalloc,
                    IARG_THREAD_ID,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                    IARG_END);

                RTN_InsertCall(
                    rtn,
                    IPOINT_AFTER,
                    (AFUNPTR)AfterMalloc,
                    IARG_THREAD_ID,
                    IARG_FUNCRET_EXITPOINT_VALUE,
                    IARG_END);

                RTN_Close(rtn);
            }

            /* ================= calloc ================= */

            else if (name.find("calloc") != std::string::npos)
            {
                RTN_Open(rtn);

                RTN_InsertCall(
                    rtn,
                    IPOINT_BEFORE,
                    (AFUNPTR)BeforeCalloc,
                    IARG_THREAD_ID,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                    IARG_END);

                RTN_InsertCall(
                    rtn,
                    IPOINT_AFTER,
                    (AFUNPTR)AfterCalloc,
                    IARG_THREAD_ID,
                    IARG_FUNCRET_EXITPOINT_VALUE,
                    IARG_END);

                RTN_Close(rtn);
            }

            /* ================= realloc ================= */

            else if (name.find("realloc") != std::string::npos)
            {
                RTN_Open(rtn);

                RTN_InsertCall(
                    rtn,
                    IPOINT_BEFORE,
                    (AFUNPTR)BeforeRealloc,
                    IARG_THREAD_ID,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                    IARG_END);

                RTN_InsertCall(
                    rtn,
                    IPOINT_AFTER,
                    (AFUNPTR)AfterRealloc,
                    IARG_THREAD_ID,
                    IARG_FUNCRET_EXITPOINT_VALUE,
                    IARG_END);

                RTN_Close(rtn);
            }

            /* ================= free ================= */

            else if (name.find("free") != std::string::npos)
            {
                RTN_Open(rtn);

                RTN_InsertCall(
                    rtn,
                    IPOINT_BEFORE,
                    (AFUNPTR)BeforeFree,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                    IARG_END);

                RTN_Close(rtn);
            }
        }
    }
}

/* ===================== FINI ===================== */

VOID Fini(INT32 code, VOID *v)
{
    outFile << "\n===== FINAL REPORT =====\n";

    if (!active_allocs.empty())
    {
        outFile << "Memory Leaks Detected:\n";

        for (auto &it : active_allocs)
        {
            outFile << "Leak at 0x"
                    << std::hex << it.first
                    << " Size: "
                    << std::dec << it.second
                    << " bytes"
                    << endl;
        }
    }
    else
    {
        outFile << "No memory leaks.\n";
    }

    outFile << "\n===== STATISTICS =====\n";

    outFile << "Malloc Calls       : "
            << malloc_count
            << endl;

    outFile << "Calloc Calls       : "
            << calloc_count
            << endl;

    outFile << "Realloc Calls      : "
            << realloc_count
            << endl;

    outFile << "Valid Frees        : "
            << free_count
            << endl;

    outFile << "Invalid Frees      : "
            << invalid_free_count
            << endl;

    outFile << "Double Frees       : "
            << double_free_count
            << endl;

    outFile << "Memory Leaks       : "
            << active_allocs.size()
            << endl;

    outFile.close();
}

/* ===================== MAIN ===================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    PIN_InitLock(&pinLock);

    if (PIN_Init(argc, argv))
        return 1;

    outFile.open("memtrace.out");

    IMG_AddInstrumentFunction(ImageLoad, 0);

    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();

    return 0;
}