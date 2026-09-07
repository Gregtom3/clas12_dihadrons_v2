# One-pass production

The production engine indexes selected particles once per event and constructs
all requested channels from that in-memory event. It never determines a channel
from an input filename.

The first implementation covers the three charged channels. Identical-pion
channels use unordered combinations, so a particle cannot pair with itself and
each physical pair appears once. The mixed-charge channel uses the Cartesian
product of selected positive and negative pions.

Neutral channels deliberately fail if sent to the charged builder. They will be
enabled through a dedicated photon-to-pi0 builder with cached model inference and
sideband metadata; this avoids accidental placeholder behavior.

HIPO decoding and ROOT writing are adapters around this tested core. The same
candidate logic therefore runs in local tests and JLab production.
