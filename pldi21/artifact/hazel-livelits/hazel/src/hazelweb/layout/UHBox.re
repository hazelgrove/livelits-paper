[@deriving sexp]
type t = Pretty.Box.t(UHAnnot.t);
include Pretty.Box.Make(WeakMap);
