#include <blocksigner.h>
#include <tpos/tposutils.h>
#include <tpos/activemerchantnode.h>
#include <keystore.h>
#include <primitives/block.h>
#include <utilstrencodings.h>
#include <messagesigner.h>

static CPubKey::InputScriptType GetScriptTypeFromDestination(const CTxDestination &dest)
{
    if (boost::get<CKeyID>(&dest)) {
        return CPubKey::InputScriptType::SPENDP2PKH;
    }
    if (boost::get<WitnessV0KeyHash>(&dest)) {
        return CPubKey::InputScriptType::SPENDWITNESS;
    }
    if (boost::get<CScriptID>(&dest)) {
        return CPubKey::InputScriptType::SPENDP2SHWITNESS;
    }
    return CPubKey::InputScriptType::SPENDUNKNOWN;
}

CBlockSigner::CBlockSigner(CBlock &block, const CKeyStore *keystore) :
    refBlock(block),
    refKeystore(keystore)
{

}

bool CBlockSigner::SignBlock()
{
    CKey keySecret;
    CPubKey::InputScriptType scriptType;

    if(refBlock.IsProofOfStake())
    {
        const CTxOut& txout = refBlock.vtx[1]->vout[1];

        CTxDestination destination;
        if(!ExtractDestination(txout.scriptPubKey, destination))
        {
            return error("Failed to extract destination while signing: %s\n", txout.ToString());
        }
        {
            auto keyid = GetKeyForDestination(*refKeystore, destination);
            if (keyid.IsNull()) {
                return error("CBlockSigner::SignBlock() : failed to get key for destination, won't sign.");
            }
            if (!refKeystore->GetKey(keyid, keySecret)) {
                return error("CBlockSigner::SignBlock() : Private key for address %s not known", EncodeDestination(destination));
            }

            scriptType = GetScriptTypeFromDestination(destination);
        }
    }
//?
    return CHashSigner::SignHash(refBlock.GetHash(), keySecret, scriptType, refBlock.vchBlockSig);
}

bool CBlockSigner::CheckBlockSignature() const
{
    if(refBlock.IsProofOfWork())
        return true;

    if(refBlock.vchBlockSig.empty())
        return false;

    const CTxOut& txout = refBlock.vtx[1]->vout[1];

    CTxDestination destination;

    if(!ExtractDestination(txout.scriptPubKey, destination))
    {
        return error("CBlockSigner::CheckBlockSignature() : failed to extract destination from script: %s", txout.scriptPubKey.ToString());
    }

    auto hashMessage = refBlock.GetHash();
    if(refBlock.IsProofOfWork())
    {
        return true;
    }

    std::string strError;
    return CHashSigner::VerifyHash(hashMessage, destination, refBlock.vchBlockSig, strError);
}
