#include <ripple/basics/Blob.h>
#include <ripple/protocol/TER.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/app/misc/Transaction.h>
#include <queue>
#include <optional>
#include <any>
#include <memory>

#include "common/value.h"
#include "vm/configure.h"
#include "vm/vm.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"


#ifndef RIPPLE_HOOK_H_INCLUDED
#define RIPPLE_HOOK_H_INCLUDED

namespace hook {
    struct HookContext;
}

namespace hook_api {

#define TER_TO_HOOK_RETURN_CODE(x)\
    (((TERtoInt(x)) << 16)*-1)


// for debugging if you want a lot of output change these to if (1)
#define DBG_PRINTF if (0) printf
#define DBG_FPRINTF if (0) fprintf

    enum api_return_code {
        SUCCESS = 0,                    // return codes > 0 are reserved for hook apis to return "success"
        OUT_OF_BOUNDS = -1,             // could not read or write to a pointer to provided by hook
        INTERNAL_ERROR = -2,            // eg directory is corrupt
        TOO_BIG = -3,                   // something you tried to store was too big
        TOO_SMALL = -4,                 // something you tried to store or provide was too small
        DOESNT_EXIST = -5,              // something you requested wasn't found
        NO_FREE_SLOTS = -6,             // when trying to load an object there is a maximum of 255 slots
        INVALID_ARGUMENT = -7,          // self explanatory
        ALREADY_SET = -8,               // returned when a one-time parameter was already set by the hook
        PREREQUISITE_NOT_MET = -9,      // returned if a required param wasn't set, before calling
        FEE_TOO_LARGE = -10,            // returned if the attempted operation would result in an absurd fee
        EMISSION_FAILURE = -11,         // returned if an emitted tx was not accepted by rippled
        TOO_MANY_NONCES = -12,          // a hook has a maximum of 256 nonces
        TOO_MANY_EMITTED_TXN = -13,     // a hook has emitted more than its stated number of emitted txn
        NOT_IMPLEMENTED = -14,          // an api was called that is reserved for a future version
        INVALID_ACCOUNT = -15,          // an api expected an account id but got something else
        GUARD_VIOLATION = -16,          // a guarded loop or function iterated over its maximum
        INVALID_FIELD = -17,            // the field requested is returning sfInvalid
        PARSE_ERROR = -18               // hook asked hookapi to parse something the contents of which was invalid
    };

    // many datatypes can be encoded into an int64_t
    int64_t data_as_int64(
            void* ptr_raw,
            uint32_t len)
    {
        unsigned char* ptr = reinterpret_cast<unsigned char*>(ptr_raw);
        if (len > 8)
            return TOO_BIG;
        uint64_t output = 0;
        for (int i = 0, j = (len-1)*8; i < len; ++i, j-=8)
            output += (((uint64_t)ptr[i]) << j);
        if ((1ULL<<63) & output)
            return TOO_BIG;
        return output;
    }

    enum ExitType : int8_t {
        UNSET = -2,
        WASM_ERROR = -1,
        ROLLBACK = 0,
        ACCEPT = 1,
    };

    const int etxn_details_size = 105;
    const int max_slots = 255;
    const int max_nonce = 255;
    const int max_emit = 255;
    const int drops_per_byte = 31250; //RH TODO make these  votable config option
    const double fee_base_multiplier = 1.1f;

    // RH TODO: consider replacing macros with templates
    #define DECLARE_HOOK_FUNCTION(R, F, ...)\
        class WasmFunction_##F : public SSVM::Runtime::HostFunction<WasmFunction_##F>\
        {\
            public:\
            hook::HookContext& hookCtx;\
            WasmFunction_##F(hook::HookContext& ctx) : hookCtx(ctx) {};\
            SSVM::Expect<R> body(SSVM::Runtime::Instance::MemoryInstance*, __VA_ARGS__);\
        }

    #define DECLARE_HOOK_FUNCNARG(R, F)\
        class WasmFunction_##F : public SSVM::Runtime::HostFunction<WasmFunction_##F>\
        {\
            public:\
            hook::HookContext& hookCtx;\
            WasmFunction_##F(hook::HookContext& ctx) : hookCtx(ctx) {};\
            SSVM::Expect<R> body(SSVM::Runtime::Instance::MemoryInstance*);\
        }

    // RH NOTE: Find descriptions of api functions in ./impl/applyHook.cpp and hookapi.h (include for hooks)

    // the "special" _() api allows every other api to be invoked by a number (crc32 of name)
    // instead of function name
    DECLARE_HOOK_FUNCTION(int64_t, special,  uint32_t api_no,
                                             uint32_t a, uint32_t b, uint32_t c,
                                             uint32_t d, uint32_t e, uint32_t f);


    DECLARE_HOOK_FUNCTION(int32_t,  _g,                 uint32_t guard_id, uint32_t maxiter );

    DECLARE_HOOK_FUNCTION(int64_t,	accept,             uint32_t read_ptr, uint32_t read_len, int32_t error_code );
    DECLARE_HOOK_FUNCTION(int64_t,	rollback,           uint32_t read_ptr, uint32_t read_len, int32_t error_code );
    DECLARE_HOOK_FUNCTION(int64_t,	util_raddr,         uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t read_ptr, uint32_t read_len );
    DECLARE_HOOK_FUNCTION(int64_t,	util_accid,         uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t read_ptr, uint32_t read_len );
    DECLARE_HOOK_FUNCTION(int64_t,	util_verify,        uint32_t dread_ptr, uint32_t dread_len,
                                                        uint32_t sread_ptr, uint32_t sread_len,
                                                        uint32_t kread_ptr, uint32_t kread_len );
    DECLARE_HOOK_FUNCTION(int64_t,	util_verify_sto,    uint32_t tread_ptr, uint32_t tread_len );
    DECLARE_HOOK_FUNCTION(int64_t,	util_sha512h,       uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t read_ptr,  uint32_t read_len );
    DECLARE_HOOK_FUNCTION(int64_t,	util_subfield,      uint32_t read_ptr, uint32_t read_len, uint32_t field_id );
    DECLARE_HOOK_FUNCTION(int64_t,	util_subarray,      uint32_t read_ptr, uint32_t read_len, uint32_t array_id );
    DECLARE_HOOK_FUNCNARG(int64_t,	etxn_burden         );
    DECLARE_HOOK_FUNCTION(int64_t,	etxn_details,       uint32_t write_ptr, uint32_t write_len );
    DECLARE_HOOK_FUNCTION(int64_t,	etxn_fee_base,      uint32_t tx_byte_count);
    DECLARE_HOOK_FUNCTION(int64_t,	etxn_reserve,       uint32_t count );
    DECLARE_HOOK_FUNCNARG(int64_t,	etxn_generation     );
    DECLARE_HOOK_FUNCTION(int64_t,	emit,               uint32_t read_ptr, uint32_t read_len );
    DECLARE_HOOK_FUNCTION(int64_t,	hook_account,       uint32_t write_ptr, uint32_t write_len );
    DECLARE_HOOK_FUNCTION(int64_t,	hook_hash,          uint32_t write_ptr, uint32_t write_len );
    DECLARE_HOOK_FUNCNARG(int64_t,	fee_base            );
    DECLARE_HOOK_FUNCNARG(int64_t,	ledger_seq          );
    DECLARE_HOOK_FUNCTION(int64_t,	nonce,              uint32_t write_ptr, uint32_t write_len );
    DECLARE_HOOK_FUNCTION(int64_t,	slot_clear,         uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	slot_set,           uint32_t read_ptr, uint32_t read_len,
                                                        uint32_t slot_type, int32_t slot );

    DECLARE_HOOK_FUNCTION(int64_t,	slot_field_txt,     uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t field_id, uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	slot_field,         uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t field_id, uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	slot_id,            uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	slot_type,          uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	state_set,          uint32_t read_ptr,  uint32_t read_len,
                                                        uint32_t kread_ptr, uint32_t kread_len );
    DECLARE_HOOK_FUNCTION(int64_t,	state,              uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t kread_ptr, uint32_t kread_len );
    DECLARE_HOOK_FUNCTION(int64_t,	state_foreign,      uint32_t write_ptr, uint32_t write_len,
                                                        uint32_t kread_ptr, uint32_t kread_len,
                                                        uint32_t aread_ptr, uint32_t aread_len );
    DECLARE_HOOK_FUNCTION(int64_t,	trace_slot,         uint32_t slot );
    DECLARE_HOOK_FUNCTION(int64_t,	trace,              uint32_t read_ptr, uint32_t read_len, uint32_t as_hex );
    DECLARE_HOOK_FUNCTION(int64_t,	trace_num,          uint32_t read_ptr, uint32_t read_len, int64_t number );

    DECLARE_HOOK_FUNCNARG(int64_t,	otxn_burden         );
    DECLARE_HOOK_FUNCTION(int64_t,	otxn_field,         uint32_t write_ptr, uint32_t write_len, uint32_t field_id );
    DECLARE_HOOK_FUNCTION(int64_t,	otxn_field_txt,     uint32_t write_ptr, uint32_t write_len, uint32_t field_id );
    DECLARE_HOOK_FUNCNARG(int64_t,	otxn_generation     );
    DECLARE_HOOK_FUNCTION(int64_t,	otxn_id,            uint32_t write_ptr, uint32_t write_len );
    DECLARE_HOOK_FUNCNARG(int64_t,	otxn_type           );

    std::set<std::string> import_whitelist
    {
        "_",
        "_g",
        "accept",
        "rollback",
        "util_raddr",
        "util_accid",
        "util_verify",
        "util_verify_sto",
        "util_sha512h",
        "util_subfield",
        "util_subarray",
        "etxn_details",
        "etxn_fee_base",
        "etxn_reserve",
        "emit",
        "hook_account",
        "hook_hash",
        "nonce",
        "slot_clear",
        "slot_set",
        "slot_field_txt",
        "slot_field",
        "slot_id",
        "slot_type",
        "state_set",
        "state",
        "state_foreign",
        "trace_slot",
        "trace",
        "trace_num",
        "otxn_field",
        "otxn_field_txt",
        "otxn_id"
    };    
   
} /* end namespace hook_api */

namespace hook {

    bool canHook(ripple::TxType txType, uint64_t hookOn);

    struct HookResult;

    HookResult apply(
            ripple::uint256,
            ripple::Blob,
            ripple::ApplyContext&,
            const ripple::AccountID&,
            bool callback);

    struct HookContext;

    int maxHookStateDataSize(void);


    struct HookResult
    {
        ripple::Keylet accountKeylet;
        ripple::Keylet ownerDirKeylet;
        ripple::Keylet hookKeylet;
        ripple::AccountID account;
        std::queue<std::shared_ptr<ripple::Transaction>> emittedTxn; // etx stored here until accept/rollback
        // uint256 key -> [ has_been_modified, current_state ]
        std::shared_ptr<std::map<ripple::uint256, std::pair<bool, ripple::Blob>>> changedState;
        hook_api::ExitType exitType = hook_api::ExitType::ROLLBACK;
        std::string exitReason {""};
        int64_t exitCode {-1};
    };
    
    class HookModule;

    struct HookContext {
        ripple::ApplyContext& applyCtx;
        // slots are used up by requesting objects from inside the hook
        // the map stores pairs consisting of a memory view and whatever shared or unique ptr is required to
        // keep the underlying object alive for the duration of the hook's execution
        std::map<int, std::pair<std::string_view, std::any>> slot;
        int slot_counter { 1 };
        std::queue<int> slot_free {};
        int64_t expected_etxn_count { -1 }; // make this a 64bit int so the uint32 from the hookapi cant overflow it
        int nonce_counter { 0 }; // incremented whenever nonce is called to ensure unique nonces
        std::map<ripple::uint256, bool> nonce_used;
        uint32_t generation = 0; // used for caching, only generated when txn_generation is called
        int64_t burden = 0;      // used for caching, only generated when txn_burden is called
        int64_t fee_base = 0;
        std::map<uint32_t, uint32_t> guard_map; // iteration guard map <id -> upto_iteration>
        HookResult result;
        const HookModule* module = 0;
    };

    // RH TODO: fetch this value from the hook sle
    int maxHookStateDataSize(void) {
        return 128;
    }

    ripple::TER
    setHookState(
        HookResult& hookResult,
        ripple::ApplyContext& applyCtx,
        ripple::Keylet const& hookStateKeylet,
        ripple::uint256 key,
        ripple::Slice& data
    );

    // finalize the changes the hook made to the ledger
    void commitChangesToLedger( hook::HookResult& hookResult, ripple::ApplyContext& );

    #define ADD_HOOK_FUNCTION(F, ctx)\
        addHostFunc(#F, std::make_unique<hook_api::WasmFunction_##F>(ctx))

    class HookModule : public SSVM::Runtime::ImportObject
    {
    
    public:
        HookContext hookCtx;

        HookModule(HookContext& ctx) : SSVM::Runtime::ImportObject("env"), hookCtx(ctx)
        {
            //addHostFunc("_", std::make_unique<hook_api::WasmFunction_special(ctx));
            ctx.module = this;

            ADD_HOOK_FUNCTION(_g, ctx);
            ADD_HOOK_FUNCTION(accept, ctx);
            ADD_HOOK_FUNCTION(rollback, ctx);
            ADD_HOOK_FUNCTION(util_raddr, ctx);
            ADD_HOOK_FUNCTION(util_accid, ctx);
            ADD_HOOK_FUNCTION(util_verify, ctx);
            ADD_HOOK_FUNCTION(util_verify_sto, ctx);
            ADD_HOOK_FUNCTION(util_sha512h, ctx);
            ADD_HOOK_FUNCTION(util_subfield, ctx);
            ADD_HOOK_FUNCTION(util_subarray, ctx);
            ADD_HOOK_FUNCTION(emit, ctx);
            ADD_HOOK_FUNCTION(etxn_burden, ctx);
            ADD_HOOK_FUNCTION(etxn_fee_base, ctx);
            ADD_HOOK_FUNCTION(etxn_details, ctx);
            ADD_HOOK_FUNCTION(etxn_reserve, ctx);
            ADD_HOOK_FUNCTION(etxn_generation, ctx);
            ADD_HOOK_FUNCTION(otxn_burden, ctx);
            ADD_HOOK_FUNCTION(otxn_generation, ctx);
            ADD_HOOK_FUNCTION(otxn_field_txt, ctx);
            ADD_HOOK_FUNCTION(otxn_field, ctx);
            ADD_HOOK_FUNCTION(otxn_id, ctx);
            ADD_HOOK_FUNCTION(otxn_type, ctx);
            ADD_HOOK_FUNCTION(hook_account, ctx);
            ADD_HOOK_FUNCTION(hook_hash, ctx);
            ADD_HOOK_FUNCTION(fee_base, ctx);
            ADD_HOOK_FUNCTION(ledger_seq, ctx);
            ADD_HOOK_FUNCTION(nonce, ctx);
            ADD_HOOK_FUNCTION(state, ctx);
            ADD_HOOK_FUNCTION(state_foreign, ctx);
            ADD_HOOK_FUNCTION(state_set, ctx);
            ADD_HOOK_FUNCTION(slot_set, ctx);
            ADD_HOOK_FUNCTION(slot_clear, ctx);
            ADD_HOOK_FUNCTION(slot_field_txt, ctx);
            ADD_HOOK_FUNCTION(slot_field, ctx);
            ADD_HOOK_FUNCTION(slot_id, ctx);
            ADD_HOOK_FUNCTION(slot_type, ctx);
            ADD_HOOK_FUNCTION(trace, ctx);
            ADD_HOOK_FUNCTION(trace_slot, ctx);
            ADD_HOOK_FUNCTION(trace_num, ctx);

            SSVM::AST::Limit TabLimit(10, 20);
            addHostTable("table", std::make_unique<SSVM::Runtime::Instance::TableInstance>(
                    SSVM::ElemType::FuncRef, TabLimit));
            SSVM::AST::Limit MemLimit(1, 1);
            addHostMemory("memory", std::make_unique<SSVM::Runtime::Instance::MemoryInstance>(MemLimit));
        }
        virtual ~HookModule() = default;
    };

}

#endif
