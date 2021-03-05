open Virtual_dom;

let root_id: string;
let focus: unit => unit;

/**
 * Code representation of UHExp.
 */
let view:
  (
    ~inject: ModelAction.t => Vdom.Event.t,
    ~font_metrics: FontMetrics.t,
    ~is_mac: bool,
    ~selected_instances: UserSelectedInstances.t,
    ~sync_livelit: ModelAction.t => unit,
    ~settings: Settings.t,
    ~llview_ctx: Statics.livelit_web_view_ctx,
    Program.t
  ) =>
  Vdom.Node.t;
