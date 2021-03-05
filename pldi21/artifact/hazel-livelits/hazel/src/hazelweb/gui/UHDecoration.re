open Virtual_dom.Vdom;

module MeasuredPosition = Pretty.MeasuredPosition;
module MeasuredLayout = Pretty.MeasuredLayout;

type rects = list(SvgUtil.Rect.t);

let rects =
    (~indent=0, ~vtrim=0.0, start: MeasuredPosition.t, m: UHMeasuredLayout.t)
    : rects => {
  let mk_rect =
      (
        ~is_first=false,
        ~is_last=false,
        start: MeasuredPosition.t,
        box: MeasuredLayout.box,
      ) =>
    SvgUtil.Rect.{
      min: {
        x: Float.of_int(start.col),
        y: Float.of_int(start.row) +. (is_first ? vtrim : 0.0),
      },
      width: Float.of_int(box.width),
      height:
        Float.of_int(box.height)
        -. (is_first ? vtrim : 0.0)
        -. (is_last ? vtrim : 0.0),
    };
  let n = List.length(m.metrics);
  m.metrics
  |> List.mapi((i, box) => (i, box))
  |> ListUtil.map_with_accumulator(
       (start: MeasuredPosition.t, (i, box: MeasuredLayout.box)) =>
         (
           {row: start.row + box.height, col: indent},
           mk_rect(~is_first=i == 0, ~is_last=i == n - 1, start, box),
         ),
       start,
     )
  |> snd;
};

module VarUse = {
  let view =
      (
        ~corner_radii: (float, float),
        (offset, subject): UHMeasuredLayout.with_offset,
      )
      : Node.t =>
    subject
    |> rects({row: 0, col: offset})
    |> SvgUtil.OrthogonalPolygon.mk(~corner_radii)
    |> SvgUtil.Path.view(
         ~attrs=
           Attr.[
             classes(["var-use"]),
             create("vector-effect", "non-scaling-stroke"),
           ],
       );
};

module CurrentTerm = {
  let inline_open_child_border_height = 0.1; // y units
  let multiline_open_child_border_width = 0.25; // x units
  let tessera_margin = 0.03; // y units

  let inline_open_child_rects =
      (start: MeasuredPosition.t, m: UHMeasuredLayout.t): rects => {
    [
      // TODO relax assumption
      // andrew commented out the below assertion as it was displeasing
      //assert(MeasuredLayout.height(m) == 1);
      // make singleton skinny rect along bottom
      SvgUtil.Rect.{
        min: {
          x: Float.of_int(start.col),
          y:
            Float.of_int(start.row)
            +. 1.
            -. inline_open_child_border_height
            -. tessera_margin,
        },
        height: inline_open_child_border_height,
        width: Float.of_int(MeasuredLayout.width(m)),
      },
    ];
  };

  let multiline_open_child_rects =
      (
        ~overflow_left,
        ~vtrim_bot=false,
        start: MeasuredPosition.t,
        m: UHMeasuredLayout.t,
      )
      : rects => {
    let overflow_left =
      overflow_left ? multiline_open_child_border_width : 0.0;
    [
      // make singleton skinny rect
      SvgUtil.Rect.{
        min: {
          x: Float.of_int(start.col) -. overflow_left,
          y: Float.of_int(start.row),
        },
        height:
          Float.of_int(MeasuredLayout.height(m))
          -. (vtrim_bot ? tessera_margin : 0.),
        width: multiline_open_child_border_width,
      },
    ];
  };

  let sort_cls: TermSort.t => string =
    fun
    | Typ => "Typ"
    | Pat => "Pat"
    | Exp => "Exp";

  let closed_child_filter = (sort: TermSort.t) => {
    let sort_cls = sort_cls(sort);
    Node.create_svg(
      "filter",
      [
        Attr.id(
          String.lowercase_ascii(sort_cls) ++ "-closed-child-drop-shadow",
        ),
      ],
      [
        Node.create_svg(
          "feOffset",
          [
            Attr.create("in", "SourceAlpha"),
            Attr.create("dx", "0.1"),
            Attr.create("dy", "0.04"),
            Attr.create("result", "offset-alpha"),
          ],
          [],
        ),
        Node.create_svg(
          "feFlood",
          [
            Attr.classes(["closed-child-inset-shadow", sort_cls]),
            Attr.create("flood-opacity", "1"),
            Attr.create("result", "color"),
          ],
          [],
        ),
        Node.create_svg(
          "feComposite",
          [
            // Attr.classes(["closed-child-drop-shadow"]),
            Attr.create("operator", "out"),
            Attr.create("in", "SourceAlpha"),
            Attr.create("in2", "offset-alpha"),
            Attr.create("result", "shadow-shape"),
          ],
          [],
        ),
        Node.create_svg(
          "feComposite",
          [
            Attr.create("operator", "in"),
            Attr.create("in", "color"),
            Attr.create("in2", "shadow-shape"),
            Attr.create("result", "drop-shadow"),
          ],
          [],
        ),
        Node.create_svg(
          "feMerge",
          [],
          [
            Node.create_svg(
              "feMergeNode",
              [Attr.create("in", "SourceGraphic")],
              [],
            ),
            Node.create_svg(
              "feMergeNode",
              [Attr.create("in", "drop-shadow")],
              [],
            ),
          ],
        ),
      ],
    );
  };

  let overflow_left: TermShape.t => bool =
    fun
    | SubBlock(_)
    | NTuple(_)
    | BinOp(_) => true
    | Case
    | Rule
    | Operand
    | FreeLivelit
    | ApLivelit
    | LivelitExpression(_) => false;

  // highlighted tesserae (ignoring closed children)
  let current_term_tessera_rects =
      (~shape: TermShape.t, (offset, subject): UHMeasuredLayout.with_offset)
      : rects =>
    subject
    |> MeasuredLayout.pos_fold(
         ~start={row: 0, col: offset},
         ~linebreak=_ => [],
         ~text=(_, _) => [],
         ~align=(_, rs) => rs,
         ~cat=(_, rs1, rs2) => rs1 @ rs2,
         ~annot=
           (~go, ~indent, ~start, annot: UHAnnot.t, m) =>
             switch (shape, annot) {
             | (Case | SubBlock(_), Step(_))
             | (Case, Term({shape: Rule, _})) => go(m)
             | (_, Tessera) =>
               rects(~indent, ~vtrim=tessera_margin, start, m)
             | _ => []
             },
       );

  // closed child "cutouts" from highlighted tesserae
  let current_term_closed_child_rects =
      (~shape: TermShape.t, (offset, subject): UHMeasuredLayout.with_offset)
      : list((TermSort.t, rects)) =>
    subject
    |> MeasuredLayout.pos_fold(
         ~start={row: 0, col: offset},
         ~linebreak=_ => [],
         ~text=(_, _) => [],
         ~align=(_, rss) => rss,
         ~cat=(_, rss1, rss2) => rss1 @ rss2,
         ~annot=
           (~go, ~indent, ~start, annot: UHAnnot.t, m) =>
             switch (shape, annot) {
             // hack for case and subblocks
             // TODO remove when we have tiles
             | (Case | SubBlock(_), Step(_))
             | (Case, Term({shape: Rule, _})) => go(m)
             | (_, Tessera) => go(m)
             | (_, ClosedChild({sort, _})) => [
                 (sort, rects(~indent, ~vtrim=0.1, start, m)),
               ]
             | _ => []
             },
       );

  // highlighted borders of open children
  let current_term_open_child_rects =
      (~shape: TermShape.t, (offset, subject): UHMeasuredLayout.with_offset)
      : rects => {
    let has_multiline_open_child =
      subject
      |> MeasuredLayout.fold(
           ~linebreak=false,
           ~text=_ => false,
           ~align=b => b,
           ~cat=(||),
           ~annot=
             (go, annot: UHAnnot.t, m) =>
               switch (shape, annot) {
               | (Case | SubBlock(_), Step(_))
               | (Case, Term({shape: Rule, _})) => go(m)
               | (_, OpenChild(Multiline)) => true
               | (_, _) => false
               },
         );
    let subject_height = MeasuredLayout.height(subject);
    subject
    |> MeasuredLayout.pos_fold(
         ~start={row: 0, col: offset},
         ~linebreak=_ => [],
         ~text=(_, _) => [],
         ~align=(_, rs) => rs,
         ~cat=(_, rs1, rs2) => rs1 @ rs2,
         ~annot=
           (~go, ~indent as _, ~start, annot: UHAnnot.t, m) => {
             // some tesserae need to be padded on left side to form
             // a straight edge with borders of neighboring multiline
             // open children
             let tessera_padding =
                 (~vtrim_top: bool, ~vtrim_bot: bool, ~overflow_left: bool) => {
               let min_x =
                 Float.of_int(start.col)
                 -. (overflow_left ? multiline_open_child_border_width : 0.0);
               let min_y =
                 Float.of_int(start.row) +. (vtrim_top ? tessera_margin : 0.);
               let height =
                 Float.of_int(MeasuredLayout.height(m))
                 -. (vtrim_top ? tessera_margin : 0.)
                 -. (vtrim_bot ? tessera_margin : 0.);
               SvgUtil.Rect.[
                 {
                   min: {
                     x: min_x,
                     y: min_y,
                   },
                   height,
                   width: multiline_open_child_border_width,
                 },
               ];
             };

             switch (shape, annot) {
             | (Case | SubBlock(_), Step(_))
             | (Case, Term({shape: Rule, _})) => go(m)
             | (_, OpenChild(InlineWithBorder)) =>
               // TODO(d) specify indent?
               inline_open_child_rects(start, m)
             | (_, OpenChild(Multiline)) =>
               // TODO(d) specify indent?
               multiline_open_child_rects(
                 ~overflow_left=overflow_left(shape),
                 ~vtrim_bot=start.row == subject_height - 1,
                 start,
                 m,
               )
             | (Case, Tessera) =>
               tessera_padding(
                 ~vtrim_top=start.row == 0,
                 ~vtrim_bot=start.row == subject_height - 1,
                 ~overflow_left=false,
               )
             | (BinOp(_), Tessera) when has_multiline_open_child =>
               tessera_padding(
                 ~vtrim_top=false,
                 ~vtrim_bot=true,
                 ~overflow_left=true,
               )
             | (NTuple({comma_indices}), Tessera)
                 when has_multiline_open_child =>
               tessera_padding(
                 ~vtrim_top=
                   switch (m.layout) {
                   | Annot(Step(step), _)
                       when step == IntUtil.min(comma_indices) =>
                     false
                   | _ => true
                   },
                 ~vtrim_bot=true,
                 ~overflow_left=true,
               )
             | (_, Tessera) when has_multiline_open_child && start.col == 0 =>
               // may need to revisit above `when` guard
               // to support layouts like
               // let _ = \x.{
               //   _
               // } in ...
               // where lambda has offset head
               tessera_padding(
                 ~vtrim_top=start.row == 0,
                 ~vtrim_bot=start.row == subject_height - 1,
                 ~overflow_left=overflow_left(shape),
               )
             | _ => []
             };
           },
       );
  };

  let view =
      (
        ~corner_radii: (float, float),
        ~sort: TermSort.t,
        ~shape: TermShape.t,
        (offset, subject): UHMeasuredLayout.with_offset,
      )
      : Node.t => {
    let highlighted = {
      let tesserae = current_term_tessera_rects(~shape, (offset, subject));
      let open_child_borders =
        current_term_open_child_rects(~shape, (offset, subject));
      tesserae
      @ open_child_borders
      |> SvgUtil.OrthogonalPolygon.mk(~corner_radii)
      |> SvgUtil.Path.view(
           ~attrs=[Attr.classes(["code-current-term", sort_cls(sort)])],
         );
    };
    let closed_children =
      (offset, subject)
      |> current_term_closed_child_rects(~shape)
      |> List.map(((sort, rs: rects)) => {
           let max_width =
             rs
             |> List.map((r: SvgUtil.Rect.t) => r.width)
             |> List.fold_left(max, 0.);
           rs
           |> List.map(r => SvgUtil.Rect.{...r, width: max_width})
           |> SvgUtil.OrthogonalPolygon.mk(~corner_radii)
           |> SvgUtil.Path.view(
                ~attrs=
                  Attr.[classes(["code-closed-child", sort_cls(sort)])],
              );
         });
    let outer_filter =
      Node.create_svg(
        "filter",
        [Attr.id("outer-drop-shadow")],
        [
          Node.create_svg(
            "feDropShadow",
            [
              Attr.classes(["current-term-drop-shadow", sort_cls(sort)]),
              Attr.create("dx", "0.1"),
              Attr.create("dy", "0.04"),
              Attr.create("stdDeviation", "0"),
            ],
            [],
          ),
        ],
      );
    Node.create_svg(
      "g",
      [],
      [
        // TODO cache filters at document root
        outer_filter,
        closed_child_filter(Typ),
        closed_child_filter(Pat),
        closed_child_filter(Exp),
        highlighted,
        ...closed_children,
      ],
    );
  };
};

module ErrHole = {
  let view =
      (
        ~contains_current_term: bool,
        ~corner_radii: (float, float),
        (offset, subject): UHMeasuredLayout.with_offset,
      )
      : Node.t =>
    subject
    |> rects(
         ~vtrim=
           contains_current_term
             ? 0.0 : CurrentTerm.inline_open_child_border_height,
         {row: 0, col: offset},
       )
    |> SvgUtil.OrthogonalPolygon.mk(~corner_radii)
    |> SvgUtil.Path.view(
         ~attrs=
           Attr.[
             classes(["err-hole"]),
             create("vector-effect", "non-scaling-stroke"),
           ],
       );
};

module VarErrHole = {
  let view =
      (
        ~contains_current_term: bool,
        ~corner_radii: (float, float),
        (offset, subject): UHMeasuredLayout.with_offset,
      )
      : Node.t =>
    subject
    |> rects(
         ~vtrim=
           contains_current_term
             ? 0.0 : CurrentTerm.inline_open_child_border_height,
         {row: 0, col: offset},
       )
    |> SvgUtil.OrthogonalPolygon.mk(~corner_radii)
    |> SvgUtil.Path.view(
         ~attrs=
           Attr.[
             classes(["var-err-hole"]),
             create("vector-effect", "non-scaling-stroke"),
           ],
       );
};

module Caret = {
  let view =
      (~font_metrics: FontMetrics.t, {row, col}: MeasuredPosition.t): Node.t => {
    Node.span(
      [
        Attr.id("caret"),
        Attr.create(
          "style",
          Printf.sprintf(
            // TODO make more robust
            "top: calc(%fpx - 1px); left: calc(%fpx - 1px);",
            Float.of_int(row) *. font_metrics.row_height,
            Float.of_int(col) *. font_metrics.col_width,
          ),
        ),
        Attr.classes(["blink"]),
      ],
      [],
    );
  };
};

module LivelitExpression = {
  let view =
      (
        shape: LivelitShape.t,
        (offset, subject): UHMeasuredLayout.with_offset,
      )
      : Node.t => {
    Node.create_svg(
      "rect",
      [
        Attr.create(
          "width",
          string_of_float(
            Float.of_int(MeasuredLayout.width(subject)) +. 0.1,
          ),
        ),
        Attr.create(
          "height",
          switch (shape) {
          | InvalidShape
          | Inline(_) => string_of_int(MeasuredLayout.height(subject))
          | MultiLine(_) =>
            string_of_float(
              Float.of_int(MeasuredLayout.height(subject)) +. 0.1,
            )
          },
        ),
        Attr.create("x", string_of_float(Float.of_int(offset) -. 0.05)),
        Attr.create(
          "y",
          switch (shape) {
          | InvalidShape
          | Inline(_) => "0.05"
          | MultiLine(_) => "0"
          },
        ),
        Attr.create("style", "fill: var(--livelit-expression-color);"),
      ],
      [],
    );
  };
};
