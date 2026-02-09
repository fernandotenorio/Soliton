import os
import random
import pickle
import hashlib

# ---------------- CONFIG ---------------- #

POOL_DIR = "../data/pool"
OUTPUT_STEM = "../data/pool_samples/bootstrap_1M"
TARGET = 1_000_000
SEED = 12345

EXCLUDE_HASH_PICKLES = []  # e.g. ["data/pool_samples/old.hashes.pkl"]

# --------------------------------------- #

random.seed(SEED)

def normalize_fen(fen):
    return " ".join(fen.split()[:4])

def fen_hash64(norm_fen):
    return int.from_bytes(
        hashlib.blake2b(norm_fen.encode(), digest_size=8).digest(),
        "little"
    )

# Load excluded hashes
excluded = set()
for pkl in EXCLUDE_HASH_PICKLES:
    with open(pkl, "rb") as f:
        excluded |= pickle.load(f)

print(f"Loaded {len(excluded)} excluded hashes")

# Collect pool files
pool_files = sorted(
    os.path.join(POOL_DIR, f)
    for f in os.listdir(POOL_DIR)
    if f.endswith(".fen")
)

assert pool_files, "No pool files found"

# Pass 1: count eligible positions
eligible = 0

for path in pool_files:
    with open(path) as f:
        for line in f:
            fen = line.strip()
            if not fen:
                continue
            h = fen_hash64(normalize_fen(fen))
            if h not in excluded:
                eligible += 1

assert eligible >= TARGET, "Not enough eligible positions in pool"

p = TARGET / eligible
print(f"Eligible positions: {eligible}")
print(f"Sampling probability: {p:.6f}")

# Pass 2: sample
os.makedirs(os.path.dirname(OUTPUT_STEM), exist_ok=True)

out_fen = OUTPUT_STEM + ".fen"
out_hash = OUTPUT_STEM + ".hashes.pkl"

kept = 0
used_hashes = set()

with open(out_fen, "w") as fout:
    for path in pool_files:
        with open(path) as f:
            for line in f:
                if kept >= TARGET:
                    break
                fen = line.strip()
                if not fen:
                    continue

                norm = normalize_fen(fen)
                h = fen_hash64(norm)

                if h in excluded or h in used_hashes:
                    continue

                if random.random() < p:
                    fout.write(fen + "\n")
                    used_hashes.add(h)
                    kept += 1
        if kept >= TARGET:
            break

with open(out_hash, "wb") as f:
    pickle.dump(used_hashes, f, protocol=pickle.HIGHEST_PROTOCOL)

print(f"Sampled {kept} positions")
print(f"Wrote:\n  {out_fen}\n  {out_hash}")
