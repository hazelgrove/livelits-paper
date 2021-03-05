open Sexplib.Std;

[@deriving sexp]
type t('a) = MetaVarMap.t(list((Environment.t, InstancePath.t, 'a)));

let empty: t('a) = (MetaVarMap.empty: t('a));

let next =
    (
      hii: t('a),
      u: MetaVar.t,
      sigma: Environment.t,
      path: InstancePath.t,
      a: 'a,
    )
    : (int, t('a)) => {
  let envs =
    hii
    |> MetaVarMap.find_opt(u)
    |> Option.fold(~none=[(sigma, path, a)], ~some=envs =>
         [(sigma, path, a), ...envs]
       );
  let hii = MetaVarMap.add(u, envs, hii);
  (List.length(envs) - 1, hii);
};

let update_environment =
    (hii: t('a), inst: NodeInstance.t, sigma: Environment.t, a: 'a): t('a) => {
  let (u, i) = inst;
  let hii =
    hii
    |> MetaVarMap.update(
         u,
         Option.map(instances => {
           let length = List.length(instances);
           ListUtil.update_nth(
             length - i - 1,
             instances,
             (inst_info: (Environment.t, InstancePath.t, 'a)) => {
               let (_, path, _) = inst_info;
               (sigma, path, a);
             },
           );
         }),
       );
  hii;
};

let num_instances = (hii: t('a), u: MetaVar.t): int =>
  switch (MetaVarMap.find_opt(u, hii)) {
  | Some(envs) => List.length(envs)
  | None => 0
  };

let default_instance = (hii: t('a), u: MetaVar.t): option((MetaVar.t, int)) =>
  switch (MetaVarMap.find_opt(u, hii)) {
  | Some(envs) =>
    switch (envs) {
    | [] => None
    | [_, ..._] => Some((u, 0))
    }
  | None => None
  };

let lookup =
    (hii: t('a), inst: NodeInstance.t)
    : option((Environment.t, InstancePath.t, 'a)) => {
  let (u, i) = inst;
  switch (MetaVarMap.find_opt(u, hii)) {
  | Some(envs) =>
    let length = List.length(envs);
    List.nth_opt(envs, length - i - 1);
  | None => None
  };
};
