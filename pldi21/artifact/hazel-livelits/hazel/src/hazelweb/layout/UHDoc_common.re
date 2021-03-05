module Doc = Pretty.Doc;
open UHDoc;

let map_root = (f: t => t, (doc, splices): with_splices): with_splices => (
  f(doc),
  splices,
);

type memoization_value('a) = {
  mutable inline_true: option('a),
  mutable inline_false: option('a),
};

let memoize =
    (f: (~memoize: bool, ~enforce_inline: bool, 'k) => 'v)
    : ((~memoize: bool, ~enforce_inline: bool, 'k) => 'v) => {
  let table: WeakMap.t('k, memoization_value('v)) = WeakMap.mk();
  (~memoize: bool, ~enforce_inline: bool, k: 'k) => (
    if (!memoize) {
      f(~memoize, ~enforce_inline, k);
    } else {
      switch (WeakMap.get(table, k)) {
      | None =>
        let v = f(~memoize, ~enforce_inline, k);
        let m =
          if (enforce_inline) {
            {inline_true: Some(v), inline_false: None};
          } else {
            {inline_false: Some(v), inline_true: None};
          };
        let _ = WeakMap.set(table, k, m);
        v;
      | Some(m: memoization_value('v)) =>
        if (enforce_inline) {
          switch (m.inline_true) {
          | Some(v) => v
          | None =>
            let v = f(~memoize, ~enforce_inline, k);
            m.inline_true = Some(v);
            v;
          };
        } else {
          switch (m.inline_false) {
          | Some(v) => v
          | None =>
            let v = f(~memoize, ~enforce_inline, k);
            m.inline_false = Some(v);
            v;
          };
        }
      };
    }: 'v
  );
};

let empty_: t = Doc.empty();
let space_: t = Doc.space();
let indent_: t = Doc.indent();
let linebreak_: t = Doc.linebreak();

module Delim = {
  let mk = (~index: int, delim_text: string): t =>
    Doc.annot(
      UHAnnot.mk_Token(
        ~len=StringUtil.utf8_length(delim_text),
        ~shape=Delim(index),
        (),
      ),
      Doc.text(delim_text),
    );

  let mk_quotation = (~index: int, delim_text: string): t =>
    Doc.annot(
      UHAnnot.mk_Token(
        ~len=StringUtil.utf8_length(delim_text),
        ~shape=Delim(index),
        (),
      ),
      Doc.annot(UHAnnot.String, Doc.text(delim_text)),
    );

  let empty_hole_doc = (hole_lbl: string): t => {
    let len = hole_lbl |> StringUtil.utf8_length;
    Doc.text(hole_lbl)
    |> Doc.annot(UHAnnot.HoleLabel({len: len}))
    |> Doc.annot(UHAnnot.mk_Token(~shape=Delim(0), ~len, ()));
  };

  let open_StringLit = (): t => mk_quotation(~index=0, "\"");
  let close_StringLit = (): t => mk_quotation(~index=1, "\"");
  let open_List = (): t => mk(~index=0, "[");
  let close_List = (): t => mk(~index=1, "]");

  let open_Subscript = (): t => mk(~index=0, "[");
  let colon_Subscript = (): t => mk(~index=1, ":");
  let close_Subscript = (): t => mk(~index=2, "]");

  let open_Parenthesized = (): t => mk(~index=0, "(");
  let close_Parenthesized = (): t => mk(~index=1, ")");

  let open_Inj = (inj_side: InjSide.t): t =>
    mk(~index=0, "inj[" ++ InjSide.to_string(inj_side) ++ "](");
  let close_Inj = (): t => mk(~index=1, ")");

  let sym_Lam = (): t => mk(~index=0, Unicode.lamSym);
  let open_Lam = (): t => mk(~index=1, ".{");
  let close_Lam = (): t => mk(~index=2, "}");

  let open_Case = (): t => mk(~index=0, "case");
  let close_Case = (): t => mk(~index=1, "end");
  let close_Case_ann = (): t => mk(~index=1, "end :");

  let bar_Rule = (): t => mk(~index=0, "|");
  let arrow_Rule = (): t => mk(~index=1, "=>");

  let let_LetLine = (): t => mk(~index=0, "let");
  let eq_LetLine = (): t => mk(~index=1, "=");
  let in_LetLine = (): t => mk(~index=2, "in");

  let abbrev_AbbrevLine = () => mk(~index=0, "let");
  let eq_AbbrevLine = () => mk(~index=1, "=");
  let in_AbbrevLine = () => mk(~index=2, "in");

  let livelit_LivelitDefLine = (): t => mk(~index=0, "livelit");
  let at_LivelitDefLine = (): t => mk(~index=1, "at");
  let open_LivelitDefLine = (): t => mk(~index=2, "{");
  let close_LivelitDefLine = (): t => mk(~index=3, "} in");

  let open_CommentLine = (): t => mk(~index=0, "#");

  let colon_Ann = (): t => mk(~index=0, ":");
};

let annot_ValidSeq = (s: string): t =>
  Doc.annot(UHAnnot.ValidSeq, Doc.text(s));
let annot_InvalidSeq = (s: string): t =>
  Doc.annot(UHAnnot.InvalidSeq, Doc.text(s));
let annot_Tessera: t => t = Doc.annot(UHAnnot.Tessera);
let annot_ClosedChild = (~is_inline: bool, ~sort: TermSort.t): (t => t) =>
  Doc.annot(UHAnnot.ClosedChild({is_inline, sort}));
let annot_Step = (step: int): (t => t) => Doc.annot(UHAnnot.Step(step));
let annot_Operand = (~sort: TermSort.t): (t => t) =>
  Doc.annot(UHAnnot.mk_Term(~sort, ~shape=Operand, ()));
let annot_Case: t => t =
  Doc.annot(UHAnnot.mk_Term(~sort=Exp, ~shape=Case, ()));

let annot_FreeLivelit =
  Doc.annot(UHAnnot.mk_Term(~sort=Exp, ~shape=TermShape.FreeLivelit, ()));
let annot_LivelitExpression = (~hd_index, ~view_shape) =>
  Doc.annot(
    UHAnnot.mk_Term(
      ~sort=Exp,
      ~shape=LivelitExpression({hd_index, view_shape}),
      (),
    ),
  );

let indent_and_align = (d: t): t => Doc.(hcats([indent_, align(d)]));

let mk_text = (~start_index=0, s: string): t =>
  Doc.annot(
    UHAnnot.mk_Token(
      ~shape=Text({start_index: start_index}),
      ~len=StringUtil.utf8_length(s),
      (),
    ),
    Doc.text(s),
  );

let mk_text_str = (~start_index=0, s: string): t => {
  Doc.annot(
    UHAnnot.mk_Token(
      ~shape=Text({start_index: start_index}),
      ~len=StringUtil.utf8_length(s),
      (),
    ),
    Doc.annot(UHAnnot.String, Doc.text(s)),
  );
};

let rec mk_text_string_rec = (~start_index=0, s: string): t =>
  if (List.length(String.split_on_char('\\', s)) <= 1) {
    mk_text_str(~start_index, s);
  } else if (StringUtil.is_empty(s)) {
    mk_text_str(~start_index, "");
  } else if (String.length(s) == 1) {
    if (s == "\\") {
      annot_InvalidSeq(s);
    } else {
      mk_text_str(~start_index, s);
    };
  } else {
    switch (String.sub(s, 0, 1)) {
    | "\\" =>
      switch (String.sub(s, 1, 1)) {
      | "b"
      | "t"
      | "r"
      | "n"
      | " "
      | "\\"
      | "\""
      | "'" =>
        Doc.hcat(
          annot_ValidSeq(String.sub(s, 0, 2)),
          mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
        )
      | "o" =>
        if (String.length(s) >= 5) {
          let ch1 = s.[2];
          let ch2 = s.[3];
          let ch3 = s.[4];
          if ((ch1 >= '0' && ch1 <= '7')
              && (ch2 >= '0' && ch2 <= '7')
              && ch3 >= '0'
              && ch3 <= '7') {
            if (ch1 <= '3') {
              Doc.hcat(
                annot_ValidSeq(String.sub(s, 0, 5)),
                mk_text_string_rec(String.sub(s, 5, String.length(s) - 5)),
              );
            } else {
              Doc.hcat(
                annot_InvalidSeq(String.sub(s, 0, 5)),
                mk_text_string_rec(String.sub(s, 5, String.length(s) - 5)),
              );
            };
          } else {
            Doc.hcat(
              annot_InvalidSeq(String.sub(s, 0, 2)),
              mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
            );
          };
        } else {
          Doc.hcat(
            annot_InvalidSeq(String.sub(s, 0, 2)),
            mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
          );
        }
      | "x" =>
        if (String.length(s) >= 4) {
          let ch1 = Char.lowercase_ascii(s.[2]);
          let ch2 = Char.lowercase_ascii(s.[3]);
          if ((ch1 >= '0' && ch1 <= '9' || ch1 >= 'a' && ch1 <= 'f')
              && (ch2 >= '0' && ch2 <= '9' || ch2 >= 'a' && ch2 <= 'f')) {
            Doc.hcat(
              annot_ValidSeq(String.sub(s, 0, 4)),
              mk_text_string_rec(String.sub(s, 4, String.length(s) - 4)),
            );
          } else {
            Doc.hcat(
              annot_InvalidSeq(String.sub(s, 0, 2)),
              mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
            );
          };
        } else {
          Doc.hcat(
            annot_InvalidSeq(String.sub(s, 0, 2)),
            mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
          );
        }
      | _ =>
        let ch1 = s.[1];
        if (String.length(s) >= 4) {
          let ch2 = s.[2];
          let ch3 = s.[3];
          if ((ch1 >= '0' && ch1 <= '9')
              && (ch2 >= '0' && ch2 <= '9')
              && ch3 >= '0'
              && ch3 <= '9') {
            if (int_of_string(String.sub(s, 1, 3)) < 256) {
              Doc.hcat(
                annot_ValidSeq(String.sub(s, 0, 4)),
                mk_text_string_rec(String.sub(s, 4, String.length(s) - 4)),
              );
            } else {
              Doc.hcat(
                annot_InvalidSeq(String.sub(s, 0, 4)),
                mk_text_string_rec(String.sub(s, 4, String.length(s) - 4)),
              );
            };
          } else {
            Doc.hcat(
              annot_InvalidSeq(String.sub(s, 0, 2)),
              mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
            );
          };
        } else {
          Doc.hcat(
            annot_InvalidSeq(String.sub(s, 0, 2)),
            mk_text_string_rec(String.sub(s, 2, String.length(s) - 2)),
          );
        };
      }
    | _ =>
      Doc.hcat(
        mk_text_str(~start_index, String.sub(s, 0, 1)),
        mk_text_string_rec(String.sub(s, 1, String.length(s) - 1)),
      )
    };
  };

let mk_text_string = (~start_index=0, s: string): t => {
  Doc.annot(
    UHAnnot.mk_Token(
      ~shape=Text({start_index: start_index}),
      ~len=StringUtil.utf8_length(s),
      (),
    ),
    mk_text_string_rec(~start_index, s),
  );
};

let mk_op = (op_text: string): t =>
  Doc.annot(
    UHAnnot.mk_Token(~len=StringUtil.utf8_length(op_text), ~shape=Op, ()),
    Doc.text(op_text),
  );

let mk_space_op: t = space_;

let user_newline: t =
  Doc.(
    hcats([
      space_,
      text(Unicode.user_newline) |> annot(UHAnnot.UserNewline),
    ])
  );

type formattable_child = (~enforce_inline: bool) => t;
type formatted_child =
  | UserNewline(t)
  | EnforcedInline(t)
  | Unformatted(formattable_child);

let pad = ((left, right), ~newline=[], child_doc) => {
  let lpadding = left == empty_ ? [] : [left];
  let rpadding = right == empty_ ? [] : [right];
  Doc.hcats(newline @ lpadding @ [child_doc] @ rpadding);
};

let pad_open_inline_child = (inline_padding, with_border, child_doc) => {
  child_doc
  |> pad(inline_padding)
  |> Doc.annot(
       UHAnnot.OpenChild(
         with_border ? InlineWithBorder : InlineWithoutBorder,
       ),
     );
};

let pad_closed_inline_child = (inline_padding, sort, child_doc) => {
  child_doc
  |> annot_ClosedChild(~is_inline=true, ~sort)
  |> pad(inline_padding);
};

let pad_open_unformatted_child =
    (multiline_padding, inline_padding, with_border, formattable_child) => {
  let inline =
    pad_open_inline_child(
      inline_padding,
      with_border,
      formattable_child(~enforce_inline=true),
    );
  let multiline =
    pad(
      multiline_padding,
      formattable_child(~enforce_inline=false)
      |> indent_and_align
      |> Doc.annot(UHAnnot.OpenChild(Multiline)),
    );
  Doc.choices([inline, multiline]);
};

let pad_closed_unformatted_child =
    (multiline_padding, inline_padding, sort, formattable_child) => {
  let inline =
    pad_closed_inline_child(
      inline_padding,
      sort,
      formattable_child(~enforce_inline=true),
    );
  let multiline =
    pad(
      multiline_padding,
      formattable_child(~enforce_inline=false)
      |> annot_ClosedChild(~is_inline=false, ~sort)
      |> indent_and_align,
    );
  Doc.choices([inline, multiline]);
};

let pad_delimited_open_child =
    (
      ~inline_padding: (t, t)=(empty_, empty_),
      ~multiline_padding: (t, t)=(empty_, empty_),
      ~with_border: bool=true,
      child: formatted_child,
    )
    : t => {
  switch (child) {
  | EnforcedInline(child_doc) =>
    pad_open_inline_child(inline_padding, with_border, child_doc)
  | UserNewline(child_doc) =>
    pad(~newline=[user_newline], multiline_padding, child_doc)
  | Unformatted(formattable_child) =>
    pad_open_unformatted_child(
      multiline_padding,
      inline_padding,
      with_border,
      formattable_child,
    )
  };
};

let pad_delimited_closed_child =
    (
      ~inline_padding: (t, t)=(empty_, empty_),
      ~multiline_padding: (t, t)=(empty_, empty_),
      ~sort: TermSort.t,
      child: formatted_child,
    ) => {
  switch (child) {
  | EnforcedInline(child_doc) =>
    pad_closed_inline_child(inline_padding, sort, child_doc)
  | UserNewline(child_doc) =>
    pad(
      ~newline=[user_newline],
      multiline_padding,
      child_doc
      |> annot_ClosedChild(~is_inline=false, ~sort)
      |> Doc.indent_and_align,
    )
  | Unformatted(formattable_child) =>
    pad_closed_unformatted_child(
      multiline_padding,
      inline_padding,
      sort,
      formattable_child,
    )
  };
};

type binOp_handler('operand, 'operator) =
  (Skel.t('operator) => t, Seq.t('operand, 'operator), Skel.t('operator)) =>
  option(t);

let def_bosh = (_, _, _) => None;

let pad_bidelimited_open_child =
  pad_delimited_open_child(~multiline_padding=(linebreak_, linebreak_));

let pad_left_delimited_open_child =
  pad_delimited_open_child(
    ~multiline_padding=(linebreak_, empty_),
    ~inline_padding=(space_, empty_),
  );

let pad_right_delimited_open_child =
  pad_delimited_open_child(
    ~multiline_padding=(empty_, linebreak_),
    ~inline_padding=(empty_, space_),
  );

let pad_closed_child =
  pad_delimited_closed_child(~multiline_padding=(linebreak_, linebreak_));

let pad_left_delimited_closed_child =
  pad_delimited_closed_child(
    ~multiline_padding=(linebreak_, empty_),
    ~inline_padding=(space_, empty_),
  );

let mk_Unit = (): t =>
  Delim.mk(~index=0, "()") |> annot_Tessera |> annot_Operand(~sort=Typ);

let mk_Bool = (): t =>
  Delim.mk(~index=0, "Bool") |> annot_Tessera |> annot_Operand(~sort=Typ);

let mk_Int = (): t =>
  Delim.mk(~index=0, "Int") |> annot_Tessera |> annot_Operand(~sort=Typ);

let mk_String = (): t =>
  Delim.mk(~index=0, "String") |> annot_Tessera |> annot_Operand(~sort=Typ);

let mk_Float = (): t =>
  Delim.mk(~index=0, "Float") |> annot_Tessera |> annot_Operand(~sort=Typ);

let hole_lbl = (u: MetaVar.t): string => string_of_int(u);
let hole_inst_lbl = (u: MetaVar.t, i: MetaVarInst.t): string =>
  StringUtil.cat([string_of_int(u), ":", string_of_int(i)]);

let mk_EmptyHole = (~sort: TermSort.t, hole_lbl: string): t =>
  Delim.empty_hole_doc(hole_lbl) |> annot_Tessera |> annot_Operand(~sort);

let mk_Wild = (): t =>
  Delim.mk(~index=0, "_") |> annot_Tessera |> annot_Operand(~sort=Pat);

let mk_InvalidText = (~sort: TermSort.t, t: string): t =>
  mk_text(t) |> annot_Tessera |> annot_Operand(~sort);

let mk_Var = (~sort: TermSort.t, x: Var.t): t =>
  mk_text(x) |> annot_Tessera |> annot_Operand(~sort);

let mk_IntLit = (~sort: TermSort.t, n: string): t =>
  mk_text(n) |> annot_Tessera |> annot_Operand(~sort);

let mk_FloatLit = (~sort: TermSort.t, f: string): t =>
  mk_text(f) |> annot_Tessera |> annot_Operand(~sort);

let mk_StringLit = (~sort: TermSort.t, s: string): t => {
  let line_docs =
    s
    |> String.split_on_char('\n')
    |> ListUtil.map_with_accumulator(
         ((start_index, line_no), line) =>
           // TODO undo manual align once we have inlined Align rendering properly
           (
             (start_index + StringUtil.utf8_length(line) + 1, line_no + 1),
             if (line_no == 0) {
               mk_text_string(~start_index, line);
             } else {
               Doc.hcats([space_, mk_text_string(~start_index, line)]);
             },
           ),
         (0, 0),
       )
    |> snd
    |> ListUtil.join(Doc.linebreak());
  Doc.(
    hcats([
      Delim.open_StringLit(),
      hcats(line_docs),
      Delim.close_StringLit(),
    ])
  )
  |> annot_Tessera
  |> annot_Operand(~sort);
};

let mk_Subscript = (target, start_, end_): t => {
  let open_subscript = Delim.open_Subscript() |> annot_Tessera;
  let close_subscript = Delim.close_Subscript() |> annot_Tessera;
  Doc.hcats([
    // TODO fix this, not bidelimited
    pad_bidelimited_open_child(target),
    open_subscript,
    pad_bidelimited_open_child(start_),
    Delim.colon_Subscript() |> annot_Tessera,
    pad_bidelimited_open_child(end_),
    close_subscript,
  ])
  |> annot_Operand(~sort=Exp);
};

let mk_BoolLit = (~sort: TermSort.t, b: bool): t =>
  mk_text(string_of_bool(b)) |> annot_Tessera |> annot_Operand(~sort);

let mk_ListNil = (~sort: TermSort.t, ()): t =>
  Delim.mk(~index=0, "[]") |> annot_Tessera |> annot_Operand(~sort);

let mk_Parenthesized = (~sort: TermSort.t, body: formatted_child): t => {
  let open_group = Delim.open_Parenthesized() |> annot_Tessera;
  let close_group = Delim.close_Parenthesized() |> annot_Tessera;
  Doc.hcats([open_group, body |> pad_bidelimited_open_child, close_group])
  |> annot_Operand(~sort);
};

let mk_List = (body: formatted_child): t => {
  let open_group = Delim.open_List() |> annot_Tessera;
  let close_group = Delim.close_List() |> annot_Tessera;
  Doc.hcats([open_group, body |> pad_bidelimited_open_child, close_group])
  |> annot_Operand(~sort=Typ);
};

let mk_Inj =
    (~sort: TermSort.t, ~inj_side: InjSide.t, body: formatted_child): t => {
  let open_group = Delim.open_Inj(inj_side) |> annot_Tessera;
  let close_group = Delim.close_Inj() |> annot_Tessera;
  Doc.hcats([open_group, body |> pad_bidelimited_open_child, close_group])
  |> annot_Operand(~sort);
};

let mk_Lam = (p: formatted_child, body: formatted_child): t => {
  let open_group = {
    let lam_delim = Delim.sym_Lam();
    let open_delim = Delim.open_Lam();
    Doc.hcats([lam_delim, p |> pad_closed_child(~sort=Pat), open_delim])
    |> annot_Tessera;
  };
  let close_group = Delim.close_Lam() |> annot_Tessera;
  Doc.hcats([open_group, body |> pad_bidelimited_open_child, close_group])
  |> annot_Operand(~sort=Exp);
};

let mk_Case = (scrut: formatted_child, rules: list(t)): t => {
  let open_group = Delim.open_Case() |> annot_Tessera;
  let close_group = Delim.close_Case() |> annot_Tessera;
  Doc.(
    vseps(
      [
        hcats([
          open_group,
          scrut |> pad_left_delimited_open_child(~with_border=false),
        ]),
        ...rules,
      ]
      @ [close_group],
    )
  )
  |> annot_Case;
};

let mk_Rule = (p: formatted_child, clause: formatted_child): t => {
  let delim_group =
    Doc.hcats([
      Delim.bar_Rule(),
      p |> pad_closed_child(~inline_padding=(space_, space_), ~sort=Pat),
      Delim.arrow_Rule(),
    ])
    |> annot_Tessera;
  Doc.hcats([
    delim_group,
    clause |> pad_left_delimited_open_child(~with_border=false),
  ])
  |> Doc.annot(UHAnnot.mk_Term(~sort=Exp, ~shape=Rule, ()));
};

let mk_FreeLivelit = (lln: LivelitName.t): t =>
  annot_FreeLivelit(annot_Tessera(mk_text(lln)));

let mk_ApLivelit = (llname: LivelitName.t): t => {
  Doc.annot(
    UHAnnot.mk_Term(~sort=Exp, ~shape=ApLivelit, ()),
    annot_Tessera(mk_text(llname)),
  );
};

/*
 livelit $? at ?ty {
 	captures ?e;
 	type model = ?ty;
 	type action = ?ty;
 	init = ?e;
 	update = ?e;
  view = ?e;
  shape =
  expand = ?e;
 } in */

let mk_LivelitDefLine =
    (
      name: string,
      expansion_type: UHDoc.t,
      captures: UHDoc.t,
      model_type: UHDoc.t,
      action_type: UHDoc.t,
      init: UHDoc.t,
      update: UHDoc.t,
      view: UHDoc.t,
      shape: UHDoc.t,
      expand: UHDoc.t,
    )
    : t => {
  open Pretty;
  let livelit_delim = Delim.livelit_LivelitDefLine();
  let at_delim = Delim.at_LivelitDefLine();
  let open_delim = Delim.open_LivelitDefLine();
  let close_delim = Delim.close_LivelitDefLine();
  let open_group =
    Doc.hseps([
      livelit_delim,
      mk_text(name),
      at_delim,
      expansion_type |> annot_ClosedChild(~is_inline=true, ~sort=Typ),
      open_delim,
    ])
    |> annot_Tessera;
  let close_group = close_delim |> annot_Tessera;
  let ll_captures =
    Doc.hcats([
      Doc.indent(),
      Doc.hseps([Doc.text("captures"), captures]),
      Doc.text(";"),
    ]);
  let ll_line = (text, thing) =>
    indent_and_align(
      Doc.hcat(
        Doc.hseps([Doc.text(text), Doc.text("="), thing]),
        Doc.text(";"),
      ),
    );
  let body_group =
    Doc.vseps([
      ll_captures,
      ll_line("model type", model_type),
      ll_line("action type", action_type),
      ll_line("init", init),
      ll_line("update", update),
      ll_line("view", view),
      ll_line("shape", shape),
      ll_line("expand", expand),
    ])
    |> Doc.annot(UHAnnot.OpenChild(Multiline));
  Doc.vseps([open_group, body_group, close_group]);
};

let mk_LetLine = (p: formatted_child, def: formatted_child): t => {
  let open_group = {
    let let_delim = Delim.let_LetLine();
    let eq_delim = Delim.eq_LetLine();
    Doc.hcats([
      let_delim,
      p |> pad_closed_child(~inline_padding=(space_, space_), ~sort=Pat),
      eq_delim,
    ])
    |> annot_Tessera;
  };
  let close_group = Delim.in_LetLine() |> annot_Tessera;
  Doc.hcats([
    open_group,
    def |> pad_bidelimited_open_child(~inline_padding=(space_, space_)),
    close_group,
  ]);
};

let mk_AbbrevLine =
    (
      lln_new: LivelitName.t,
      lln_old: LivelitName.t,
      args: list(formatted_child),
    )
    : t => {
  let open_group = {
    let abbrev_delim = Delim.abbrev_AbbrevLine();
    let eq_delim = Delim.eq_AbbrevLine();
    let lln_new_doc = Doc.hseps([abbrev_delim, mk_text(lln_new), eq_delim]);
    lln_new_doc |> annot_Tessera;
  };
  let inline_choice = {
    let args =
      args
      |> List.map(
           fun
           | UserNewline(_) => Doc.fail()
           | EnforcedInline(arg) => arg
           | Unformatted(arg) => arg(~enforce_inline=true),
         );
    Doc.hseps(args);
  };
  let old_ll_group =
    Doc.hcats([
      space_,
      Doc.hseps([
        mk_text(~start_index=LivelitName.length(lln_new) + 1, lln_old),
        inline_choice,
      ]),
      space_,
    ])
    |> Doc.annot(UHAnnot.OpenChild(InlineWithBorder));
  let close_group = Delim.in_AbbrevLine() |> annot_Tessera;
  Doc.hcats([open_group, old_ll_group, close_group]);
};

let mk_TypeAnn =
    (~sort: TermSort.t, op: formatted_child, ann: formatted_child): t => {
  Doc.hcats([
    op |> pad_right_delimited_open_child,
    Doc.hcats([
      Delim.colon_Ann(),
      ann |> pad_left_delimited_closed_child(~sort=Typ),
    ])
    |> annot_Tessera,
  ])
  |> annot_Operand(~sort);
};

let pad_operator =
    (~inline_padding as (left, right): (t, t), operator: t): t => {
  open Doc;
  let ldoc = left == empty_ ? empty_ : left;
  let rdoc = right == empty_ ? empty_ : right;
  choices([
    hcats([ldoc, operator, rdoc]),
    hcats([linebreak(), operator, rdoc]),
  ]);
};

let rec mk_BinOp =
        (
          ~sort: TermSort.t,
          ~mk_operand: (~enforce_inline: bool, 'operand) => t,
          ~mk_operator: 'operator => t,
          ~inline_padding_of_operator: 'operator => (t, t),
          ~enforce_inline: bool,
          ~enforce_multiline=false,
          ~check_livelit_skel:
             (Seq.t('operand, 'operator), Skel.t('operator)) =>
             option(LivelitUtil.llctordata)=(_, _) => None,
          ~seq: Seq.t('operand, 'operator),
          ~llview_ctx: Statics.livelit_web_view_ctx,
          skel: Skel.t('operator),
        )
        : t => {
  let go =
    mk_BinOp(
      ~sort,
      ~mk_operand,
      ~mk_operator,
      ~inline_padding_of_operator,
      ~seq,
      ~llview_ctx,
    );

  switch (check_livelit_skel(seq, skel)) {
  | Some(ApLivelitData(llu, base_llname, llname, model, _)) =>
    let ctx = Livelits.initial_livelit_view_ctx;
    let shape =
      switch (MetaVarMap.find(llu, llview_ctx)) {
      | (_, shape) => shape
      | exception Not_found =>
        switch (VarMap.lookup(ctx, base_llname)) {
        | None => failwith("livelit " ++ base_llname ++ " not found")
        | Some((_, shape_fn)) => shape_fn(model)
        }
      };
    let hd_step = Skel.leftmost_tm_index(skel);
    let llexp =
      annot_LivelitExpression(
        ~hd_index=hd_step,
        ~view_shape=shape,
        go(~enforce_inline, skel),
      );
    let annot_LivelitView =
      Doc.annot(
        UHAnnot.LivelitView({
          llu,
          base_llname,
          llname,
          shape,
          model,
          hd_step,
        }),
      );
    switch (shape) {
    | InvalidShape => Doc.hcat(llexp, Doc.text("\xC2\xA0Invalid Shape"))
    | Inline(width) =>
      let spaceholder = Doc.text(StringUtil.replicat(width, Unicode.nbsp));
      Doc.hcat(llexp, annot_LivelitView(spaceholder));
    | MultiLine(height) =>
      if (enforce_inline) {
        Doc.fail();
      } else {
        let spaceholder =
          Doc.(
            hcats(
              Doc.text(Unicode.nbsp)
              |> ListUtil.replicate(height)
              |> ListUtil.join(Doc.linebreak()),
            )
          );
        Doc.vsep(llexp, annot_LivelitView(spaceholder));
      }
    };
  | _ =>
    switch (skel) {
    | Placeholder(n) =>
      let operand = Seq.nth_operand(n, seq);
      annot_Step(n, mk_operand(~enforce_inline, operand));
    | BinOp(_, op, skel1, skel2) =>
      let op_index = Skel.rightmost_tm_index(skel1) + Seq.length(seq);
      let (lpadding, rpadding) = {
        let (l, r) = inline_padding_of_operator(op);
        (l == empty_ ? [] : [l], r == empty_ ? [] : [r]);
      };
      let op = annot_Tessera(annot_Step(op_index, mk_operator(op)));
      let go = go(~check_livelit_skel);
      let inline_choice = {
        let skel1 = go(~enforce_inline=true, skel1);
        let skel2 = go(~enforce_inline=true, skel2);
        Doc.(
          hcats([
            annot(
              UHAnnot.OpenChild(InlineWithBorder),
              hcats([skel1, ...lpadding]),
            ),
            op,
            annot(
              UHAnnot.OpenChild(InlineWithBorder),
              hcats(rpadding @ [skel2]),
            ),
          ])
        );
      };
      let multiline_choice = {
        let skel1 = go(~enforce_inline=false, ~enforce_multiline=true, skel1);
        let skel2 = go(~enforce_inline=false, ~enforce_multiline=true, skel2);
        Doc.(
          vsep(
            annot(UHAnnot.OpenChild(Multiline), align(skel1)),
            hcats([
              op,
              annot(
                UHAnnot.OpenChild(Multiline),
                hcats(rpadding @ [align(skel2)]),
              ),
            ]),
          )
        );
      };
      let choices =
        if (enforce_inline) {
          inline_choice;
        } else if (enforce_multiline) {
          multiline_choice;
        } else {
          Doc.choice(inline_choice, multiline_choice);
        };
      Doc.annot(
        UHAnnot.mk_Term(~sort, ~shape=BinOp({op_index: op_index}), ()),
        choices,
      );
    }
  };
};

let mk_NTuple =
    (
      ~sort: TermSort.t,
      ~get_tuple_elements: Skel.t('operator) => list(Skel.t('operator)),
      ~mk_operand: (~enforce_inline: bool, 'operand) => t,
      ~mk_operator: 'operator => t,
      ~inline_padding_of_operator: 'operator => (t, t),
      ~enforce_inline: bool,
      ~llview_ctx: Statics.livelit_web_view_ctx,
      ~check_livelit_skel:
         (Seq.t('operand, 'operator), Skel.t('operator)) =>
         option(LivelitUtil.llctordata)=(_, _) => None,
      OpSeq(skel, seq): OpSeq.t('operand, 'operator),
    )
    : t => {
  let mk_BinOp =
    mk_BinOp(
      ~sort,
      ~mk_operand,
      ~mk_operator,
      ~inline_padding_of_operator,
      ~check_livelit_skel,
      ~seq,
      ~llview_ctx,
    );

  switch (get_tuple_elements(skel)) {
  | [] => failwith(__LOC__ ++ ": found empty tuple")
  | [singleton] => mk_BinOp(~enforce_inline, singleton)
  | [hd, ...tl] =>
    let hd_doc = (~enforce_inline: bool) =>
      // TODO need to relax is_inline
      Doc.annot(
        UHAnnot.OpenChild(enforce_inline ? InlineWithBorder : Multiline),
        mk_BinOp(~enforce_inline, hd),
      );
    let comma_doc = (step: int) => annot_Step(step, mk_op(","));
    let (inline_choice, comma_indices) =
      tl
      |> List.fold_left(
           ((tuple, comma_indices), elem) => {
             let comma_index =
               Skel.leftmost_tm_index(elem) - 1 + Seq.length(seq);
             let elem_doc = mk_BinOp(~enforce_inline=true, elem);
             let doc =
               Doc.hcats([
                 tuple,
                 annot_Tessera(comma_doc(comma_index)),
                 Doc.annot(
                   UHAnnot.OpenChild(InlineWithBorder),
                   Doc.hcat(space_, elem_doc),
                 ),
               ]);
             (doc, [comma_index, ...comma_indices]);
           },
           (hd_doc(~enforce_inline=true), []),
         );
    let multiline_choice =
      tl
      |> List.fold_left(
           (tuple, elem) => {
             let comma_index =
               Skel.leftmost_tm_index(elem) - 1 + Seq.length(seq);
             let elem_doc = mk_BinOp(~enforce_inline=false, elem);
             Doc.(
               vsep(
                 tuple,
                 hcat(
                   annot_Tessera(comma_doc(comma_index)),
                   // TODO need to have a choice here for multiline vs not
                   annot(
                     UHAnnot.OpenChild(Multiline),
                     hcat(space_, align(elem_doc)),
                   ),
                 ),
               )
             );
           },
           hd_doc(~enforce_inline=false),
         );
    let choices =
      enforce_inline
        ? inline_choice : Doc.choice(inline_choice, multiline_choice);
    Doc.annot(
      UHAnnot.mk_Term(
        ~sort,
        ~shape=NTuple({comma_indices: comma_indices}),
        (),
      ),
      choices,
    );
  };
};
