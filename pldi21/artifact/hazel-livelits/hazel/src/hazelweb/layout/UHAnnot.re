open Sexplib.Std;

[@deriving sexp]
type term_data = {
  shape: TermShape.t,
  sort: TermSort.t,
};
[@deriving sexp]
type token_shape =
  | Text({start_index: CharIndex.t})
  | Op
  | Delim(DelimIndex.t);
[@deriving sexp]
type token_data = {
  shape: token_shape,
  len: int,
};

[@deriving sexp]
type open_child_format =
  | InlineWithoutBorder
  | InlineWithBorder
  | Multiline;

[@deriving sexp]
type t =
  | HoleLabel({len: int})
  | Token(token_data)
  | UserNewline
  | OpenChild(open_child_format)
  | ClosedChild({
      // TODO consider whether necessary
      is_inline: bool,
      sort: TermSort.t,
    })
  | Tessera
  | CommentLine
  | Step(int)
  | Term(term_data)
  | LivelitView({
      llu: MetaVar.t,
      base_llname: LivelitName.t,
      llname: LivelitName.t,
      shape: LivelitShape.t,
      model: SerializedModel.t,
      hd_step: int,
    })
  | ValidSeq
  | InvalidSeq
  | String;

let mk_Token = (~len: int, ~shape: token_shape, ()) => Token({len, shape});

let mk_Term = (~shape: TermShape.t, ~sort: TermSort.t, ()): t =>
  Term({shape, sort});
