class Tx
{
public:
    Tx() : m_vout{3} {}

    int First() const { return m_vout; }

    // Reads are rewritten to observer calls; a write would be rewritten too and fail to compile.
    int m_vout;
    int vout() const { return m_vout; }
};

struct Decoy { int vout{0}; };
int Read(const Tx& tx) { return tx.vout(); }
int ReadAndWriteDecoy(Decoy& decoy)
{
    decoy.vout = 4;
    return decoy.vout;
}
