require('./utils-tests.js').TestRig('ws://localhost:6005').then(t=>
{
    const account =  t.randomAccount();
    t.fundFromGenesis(account).then(()=>
    {
        t.api.submit(
        {
            Account: account.classicAddress,
            TransactionType: "SetHook",
            Hooks: [
                {
                    Hook: {
                        CreateCode: t.wasm('makestate.wasm'),
                        HookApiVersion: 0,
                        HookNamespace: "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF",
                        HookOn: "0000000000000000"
                    }
                },
                {   Hook: {} },
                {   Hook: {} },
                {    Hook: {
                        CreateCode: t.wasm('checkstate.wasm'),
                        HookApiVersion: 0,
                        HookNamespace: "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF",
                        HookOn: "0000000000000000"
                    }
                }
            ],
            Fee: "100000"
        }, {wallet: account}).then(x=>
        {
            t.assertTxnSuccess(x)
                console.log(x);

                t.api.submit(
                {
                    Account: account.classicAddress,
                    TransactionType: "SetHook",  
                    Fee: "100000",
                    Hooks: [
                        {
                            Hook: {
                                Flags: t.hsfNSDELETE,
                                HookNamespace: "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
                            }
                        }
                    ]
                }, {wallet: account}).then(x=>
                {
                    t.assertTxnSuccess(x)
                    console.log(x);

                    process.exit(0);
                }).catch(t.err);
        }).catch(t.err);
    }).catch(t.err);
})



