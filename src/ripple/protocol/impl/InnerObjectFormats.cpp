//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <ripple/protocol/InnerObjectFormats.h>

namespace ripple {

InnerObjectFormats::InnerObjectFormats()
{
    add(sfSignerEntry.jsonName.c_str(),
        sfSignerEntry.getCode(),
        {
            {sfAccount, soeREQUIRED},
            {sfSignerWeight, soeREQUIRED},
        });

    add(sfSigner.jsonName.c_str(),
        sfSigner.getCode(),
        {
            {sfAccount, soeREQUIRED},
            {sfSigningPubKey, soeREQUIRED},
            {sfTxnSignature, soeREQUIRED},
        });

    add(sfHookSet.jsonName.c_str(),
        sfHookSet.getCode(),
        {
            {sfHookSetOperation,soeREQUIRED},
            {sfHookSequence,    soeOPTIONAL},
            {sfHookOn,          soeOPTIONAL},
            {sfHookReorder,     soeOPTIONAL},
            {sfFlags,           soeOPTIONAL},
            {sfHookNamespace,   soeOPTIONAL},
            {sfHookHash,        soeOPTIONAL},
            {sfHookParameters,  soeOPTIONAL},
            {sfHookDefinition,  soeOPTIONAL}
        });

    add(sfHookDefinition.jsonName.c_str(),
        sfHookDefinition.getCode(),
        {
            {sfHookOn,          soeREQUIRED},
            {sfHookNamespace,   soeREQUIRED},
            {sfHookParameters,  soeREQUIRED},
            {sfHookApiVersion,  soeREQUIRED},
            {sfCreateCode,      soeREQUIRED},
        }); 

    add(sfHookParameter.jsonName.c_str(),
        sfHookParameter.getCode(),
        {
            {sfHookParameterName,   soeREQUIRED},
            {sfHookParameterValue,  soeREQUIRED},
        });

}

InnerObjectFormats const&
InnerObjectFormats::getInstance()
{
    static InnerObjectFormats instance;
    return instance;
}

SOTemplate const*
InnerObjectFormats::findSOTemplateBySField(SField const& sField) const
{
    auto itemPtr = findByType(sField.getCode());
    if (itemPtr)
        return &(itemPtr->getSOTemplate());

    return nullptr;
}

}  // namespace ripple
