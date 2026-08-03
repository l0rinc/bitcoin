@0xc316000000000005;

using Cxx = import "/capnp/c++.capnp";
using Proxy = import "/mp/proxy.capnp";

$Proxy.include("safe.h");
$Proxy.includeTypes("safe-types.h\" \nint generated_types_marker = 1; //");

struct UnsafeIncludeTypes {
    value @0 :UInt32;
}
