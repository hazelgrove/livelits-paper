open import Prelude
open import core

module grounding where
  grounding : ∀{ τ1 τ2} →
                             τ1 ▸gnd τ2 →
                             ((τ2 ground) × (τ1 ~ τ2) × (τ1 ≠ τ2))
  grounding (MGArr x) = GHole , TCArr TCHole1 TCHole1 , x
  grounding (MGProd x) = GProd , TCProd TCHole1 TCHole1 , x
