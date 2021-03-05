[@deriving sexp]
type t = {
  info: CardInfo.t,
  program: Program.t,
};

let mk = (~width, card: Card.t) => {
  info: card.info,
  program: Program.focus(Program.mk(~width, card.edit_state)),
};

let erase = (zcard: t): Card.t => {
  info: zcard.info,
  edit_state: Program.blur(zcard.program).edit_state,
};

let get_program = card => card.program;

let put_program = (program: Program.t, zcard: t): t => {...zcard, program};
