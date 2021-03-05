module Doc = Pretty.Doc;
open DHDoc;

type formattable_child = (~enforce_inline: bool) => t;

let precedence_const = Operators_Exp.precedence_const;
let precedence_Ap = Operators_Exp.precedence_Ap;
// TODO make consistent with precedence system
let precedence_Subscript = 1;
let precedence_Times = Operators_Exp.precedence(Times);
let precedence_Divide = Operators_Exp.precedence(Divide);
let precedence_Plus = Operators_Exp.precedence(Plus);
let precedence_Minus = Operators_Exp.precedence(Minus);
let precedence_Cons = Operators_Exp.precedence(Cons);
let precedence_Caret = Operators_Exp.precedence(Caret);
let precedence_Equals = Operators_Exp.precedence(Equals);
let precedence_LessThan = Operators_Exp.precedence(LessThan);
let precedence_GreaterThan = Operators_Exp.precedence(GreaterThan);
let precedence_And = Operators_Exp.precedence(And);
let precedence_Or = Operators_Exp.precedence(Or);
let precedence_Comma = Operators_Exp.precedence(Comma);
let precedence_max = Operators_Exp.precedence_max;

let pad_child =
    (
      ~inline_padding as (l, r)=(Doc.empty(), Doc.empty()),
      ~enforce_inline: bool,
      child: formattable_child,
    )
    : t => {
  let inline_choice = Doc.hcats([l, child(~enforce_inline=true), r]);
  let para_choice =
    Doc.(
      hcats([
        linebreak(),
        indent_and_align(child(~enforce_inline=false)),
        linebreak(),
      ])
    );
  enforce_inline ? inline_choice : Doc.choice(inline_choice, para_choice);
};

module Delim = {
  let mk = (delim_text: string): t =>
    Doc.text(delim_text) |> Doc.annot(DHAnnot.Delim);

  let inst_label = ((u, i): NodeInstance.t): string => {
    StringUtil.cat([string_of_int(u + 1), ":", string_of_int(i + 1)]);
  };

  let empty_hole = (inst: NodeInstance.t): t => {
    let lbl = inst_label(inst);
    Doc.text(lbl)
    |> Doc.annot(DHAnnot.HoleLabel)
    |> Doc.annot(DHAnnot.Delim);
  };

  let free_livelit = (lln: LivelitName.t, inst: NodeInstance.t): t => {
    let lbl = inst_label(inst);
    let lbldoc =
      Doc.text(lbl)
      |> Doc.annot(DHAnnot.FreeLivelitLabel)
      |> Doc.annot(DHAnnot.Delim);
    Doc.(hcats([text(lln), lbldoc]));
  };

  let list_nil = mk("[]");
  let triv = mk("()");
  let wild = mk("_");

  let open_Parenthesized = mk("(");
  let close_Parenthesized = mk(")");

  let open_StringLit = mk("\"");
  let close_StringLit = mk("\"");

  let sym_Lam = mk(Unicode.lamSym);
  let colon_Lam = mk(":");
  let open_Lam = mk(".{");
  let close_Lam = mk("}");

  let fix_FixF = mk("fix");
  let colon_FixF = mk(":");
  let open_FixF = mk(".{");
  let close_FixF = mk("}");

  let open_Inj = (inj_side: InjSide.t) =>
    mk(StringUtil.cat([InjSide.to_string(inj_side), "("]));
  let close_Inj = mk(")");

  let open_Case = mk("case");
  let close_Case = mk("end");

  let bar_Rule = mk("|");
  let arrow_Rule = mk("=>");

  let open_Cast = mk("<");
  let arrow_Cast = mk(Unicode.castArrowSym);
  let close_Cast = mk(">");

  let open_FailedCast = open_Cast |> Doc.annot(DHAnnot.FailedCastDelim);
  let arrow_FailedCast =
    mk(Unicode.castArrowSym) |> Doc.annot(DHAnnot.FailedCastDelim);
  let close_FailedCast = close_Cast |> Doc.annot(DHAnnot.FailedCastDelim);
};

let mk_EmptyHole = (~selected=false, (u, i)) =>
  Delim.empty_hole((u, i))
  |> Doc.annot(DHAnnot.EmptyHole(selected, (u, i)));

let mk_Keyword = (u, i, k) =>
  Doc.text(ExpandingKeyword.to_string(k))
  |> Doc.annot(DHAnnot.VarHole(Keyword(k), (u, i)));

let mk_FreeLivelit = (lln, u, i) =>
  Delim.free_livelit(lln, (u, i))
  |> Doc.annot(DHAnnot.FreeLivelit(lln, (u, i)));

let mk_InvalidText = (t, (u, i)) =>
  Doc.text(t) |> Doc.annot(DHAnnot.Invalid((u, i)));

let mk_IntLit = n => Doc.text(string_of_int(n));

let mk_FloatLit = (f: float) =>
  switch (f < 0., Float.is_infinite(f), Float.is_nan(f)) {
  | (false, true, _) => Doc.text("Inf")
  /* TODO: NegInf is temporarily introduced until unary minus is introduced to Hazel */
  | (true, true, _) => Doc.text("NegInf")
  | (_, _, true) => Doc.text("NaN")
  | _ => Doc.text(string_of_float(f))
  };

let mk_BoolLit = b => Doc.text(string_of_bool(b));

let mk_StringLit = s => {
  let line_docs =
    s
    |> String.split_on_char('\n')
    |> ListUtil.map_with_accumulator(
         (line_no, line) =>
           // TODO undo manual align once we have inlined Align rendering properly
           (
             line_no + 1,
             line_no == 0
               ? Doc.text(line) : Doc.hcat(Doc.space(), Doc.text(line)),
           ),
         0,
       )
    |> snd
    |> ListUtil.join(Doc.linebreak());

  Doc.hcats([Delim.open_StringLit, ...line_docs] @ [Delim.close_StringLit])
  |> Doc.annot(DHAnnot.String);
};

let mk_Inj = (inj_side, padded_child) =>
  Doc.hcats([Delim.open_Inj(inj_side), padded_child, Delim.close_Inj]);

let mk_Cons = (hd, tl) => Doc.(hcats([hd, text("::"), tl]));

let mk_Pair = (doc1, doc2) =>
  Doc.(hcats([doc1, text(", ("), doc2, text(")")]));

let mk_Ap = (doc1, doc2) => Doc.hseps([doc1, doc2]);
