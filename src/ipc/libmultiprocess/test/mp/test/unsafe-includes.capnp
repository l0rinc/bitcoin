@0xc316000000000004;

using Cxx = import "/capnp/c++.capnp";
using Proxy = import "/mp/proxy.capnp";

$Proxy.include("safe.h\" \nint generated_marker = 1; //");
$Proxy.includeTypes("safe-types.h");

struct UnsafeInclude {
    value @0 :UInt32;
}
