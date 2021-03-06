open Sexplib.Std;

[@deriving sexp]
type t =
  | Underscore
  | IntLit(string)
  | FloatLit(string)
  | BoolLit(bool)
  | ExpandingKeyword(ExpandingKeyword.t)
  | Var(Var.t)
  | InvalidTextShape(string)
  | LivelitName(LivelitName.t);

let is_reserved_var = s => {
  let reserved_prefix = SpliceInfo.splice_var_prefix;
  let slen = String.length(s);
  let plen = String.length(reserved_prefix);
  if (slen >= plen) {
    String.equal(reserved_prefix, String.sub(s, 0, plen));
  } else {
    false;
  };
};

/* Eventually replace Ocaml's ___of_string_opt with our own rules */
/* Ocaml accepts _1 as a float */
let hazel_float_of_string_opt = (s: string): option(float) =>
  if (String.length(s) > 0 && s.[0] == '_') {
    None;
  } else {
    switch (s, String.lowercase_ascii(s)) {
    | ("NaN", _) => Some(nan)
    | ("Inf", _) => Some(infinity)
    /* TODO: NegInf is temporarily introduced until unary minus is introduced to Hazel */
    | ("NegInf", _) => Some(neg_infinity)
    | (_, "nan")
    | (_, "inf")
    | (_, "infinity") => None
    | _ => float_of_string_opt(s)
    };
  };

let of_text = (text: string): t =>
  switch (
    int_of_string_opt(text),
    hazel_float_of_string_opt(text),
    bool_of_string_opt(text),
    ExpandingKeyword.mk(text),
  ) {
  | (Some(_), _, _, _) => IntLit(text)
  | (_, Some(_), _, _) => FloatLit(text)
  | (_, _, Some(b), _) => BoolLit(b)
  | (_, _, _, Some(k)) => ExpandingKeyword(k)
  | (None, None, None, None) =>
    if (text |> String.equal("_")) {
      Underscore;
    } else if (LivelitName.is_valid_free_livelit_name(text)) {
      LivelitName(text);
    } else if (Var.is_valid(text) && !is_reserved_var(text)) {
      Var(text);
    } else {
      InvalidTextShape(text);
    }
  };
