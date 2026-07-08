class Tx
{
public:
    Tx() : vout{3} {}

    int First() const { return vout; }

    // Reads are rewritten to observer calls; a write would be rewritten too and fail to compile.
    int vout;
};

struct Decoy { int vout{0}; };
int Read(const Tx& tx) { return tx.vout; }
int ReadAndWriteDecoy(Decoy& decoy)
{
    decoy.vout = 4;
    return decoy.vout;
}
