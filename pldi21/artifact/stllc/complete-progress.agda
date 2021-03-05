open import Nat
open import Prelude
open import core
open import contexts

open import progress
open import lemmas-complete

module complete-progress where
  -- as in progress, we define a datatype for the possible outcomes of
  -- progress for readability.
  data okc : (d : iexp) (Δ : hctx) → Set where
    V : ∀{d Δ} → d val → okc d Δ
    S : ∀{d Δ} → Σ[ d' ∈ iexp ] (d ↦ d') → okc d Δ

  complete-progress : {Δ : hctx} {d : iexp} {τ : typ} →
                       Δ , ∅ ⊢ d :: τ →
                       d dcomplete →
                       okc d Δ
  complete-progress wt comp with progress wt
  complete-progress wt comp | S x = S x
  complete-progress wt comp | I x = abort (lem-ind-comp comp x)
  complete-progress wt comp | BV x = V (lem-comp-boxed-val wt comp x)
