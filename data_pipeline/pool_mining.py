import os
import random
import chess
import hashlib
from collections import Counter, defaultdict

# ---------------- CONFIG ---------------- #

POOL_DIR = "../data/pool"
FILES_SAMPLE_RATE = 0.05     # sample 5% of each file
MAX_POSITIONS = 2_000_000    # global hard cap
HASH_SAMPLE_RATE = 0.1       # only hash 10% for dup estimation
RANDOM_SEED = 42

random.seed(RANDOM_SEED)

# ---------------------------------------- #

piece_hist = Counter()
stm_hist = Counter()
queen_hist = Counter()
rook_hist = Counter()
minor_hist = Counter()
phase_hist = Counter()

file_piece_hist = defaultdict(Counter)

seen_hashes = set()
hash_seen = 0
hash_dupes = 0

total_positions = 0

# NNUE-style phase heuristic
def phase_index(board):
    return (
        4 * len(board.pieces(chess.QUEEN, chess.WHITE))
        + 4 * len(board.pieces(chess.QUEEN, chess.BLACK))
        + 2 * len(board.pieces(chess.ROOK, chess.WHITE))
        + 2 * len(board.pieces(chess.ROOK, chess.BLACK))
        + len(board.pieces(chess.BISHOP, chess.WHITE))
        + len(board.pieces(chess.BISHOP, chess.BLACK))
        + len(board.pieces(chess.KNIGHT, chess.WHITE))
        + len(board.pieces(chess.KNIGHT, chess.BLACK))
    )

def fen_hash(fen):
    return hashlib.blake2b(fen.encode(), digest_size=8).digest()

pool_files = sorted(
    os.path.join(POOL_DIR, f)
    for f in os.listdir(POOL_DIR)
    if f.endswith(".fen")
)

print(f"Scanning {len(pool_files)} pool files...")

for path in pool_files:
    with open(path) as f:
        for line in f:
            if total_positions >= MAX_POSITIONS:
                break

            if random.random() > FILES_SAMPLE_RATE:
                continue

            fen = line.strip()
            if not fen:
                continue

            try:
                board = chess.Board(fen)
            except Exception:
                continue

            total_positions += 1

            # ---------------- HISTOGRAMS ---------------- #

            pc = len(board.piece_map())
            piece_hist[pc] += 1
            file_piece_hist[path][pc] += 1

            stm_hist[board.turn] += 1

            queen_hist[
                len(board.pieces(chess.QUEEN, chess.WHITE))
                + len(board.pieces(chess.QUEEN, chess.BLACK))
            ] += 1

            rook_hist[
                len(board.pieces(chess.ROOK, chess.WHITE))
                + len(board.pieces(chess.ROOK, chess.BLACK))
            ] += 1

            minor_hist[
                len(board.pieces(chess.BISHOP, chess.WHITE))
                + len(board.pieces(chess.BISHOP, chess.BLACK))
                + len(board.pieces(chess.KNIGHT, chess.WHITE))
                + len(board.pieces(chess.KNIGHT, chess.BLACK))
            ] += 1

            phase_hist[phase_index(board)] += 1

            # ---------------- DUPLICATE ESTIMATION ---------------- #

            if random.random() < HASH_SAMPLE_RATE:
                h = fen_hash(fen)
                hash_seen += 1
                if h in seen_hashes:
                    hash_dupes += 1
                else:
                    seen_hashes.add(h)

    if total_positions >= MAX_POSITIONS:
        break

# ---------------- REPORT ---------------- #

def pct(x):
    return 100.0 * x / max(1, total_positions)

print("\n========== GLOBAL SUMMARY ==========")
print(f"Sampled positions: {total_positions}")

print("\nSide to move:")
for k in stm_hist:
    print(f"  {'White' if k else 'Black'}: {pct(stm_hist[k]):.2f}%")

print("\nTotal piece count (top 10):")
for k, v in piece_hist.most_common(10):
    print(f"  {k:2d} pieces: {pct(v):.2f}%")

print("\nQueen count:")
for k in sorted(queen_hist):
    print(f"  {k}: {pct(queen_hist[k]):.2f}%")

print("\nRook count:")
for k in sorted(rook_hist):
    print(f"  {k}: {pct(rook_hist[k]):.2f}%")

print("\nMinor piece count:")
for k in sorted(minor_hist):
    print(f"  {k}: {pct(minor_hist[k]):.2f}%")

print("\nPhase index (collapsed):")
for k, v in sorted(phase_hist.items()):
    if v > total_positions * 0.01:
        print(f"  {k}: {pct(v):.2f}%")

if hash_seen > 0:
    dup_rate = 100.0 * hash_dupes / hash_seen
    print(f"\nEstimated duplicate rate: {dup_rate:.2f}%")
else:
    print("\nDuplicate rate: insufficient data")

# ---------------- PER-FILE DIAGNOSIS ---------------- #

print("\n========== PER-FILE PIECE BIAS ==========")
for path in pool_files:
    hist = file_piece_hist[path]
    if not hist:
        continue
    avg_pieces = sum(k * v for k, v in hist.items()) / sum(hist.values())
    print(f"{os.path.basename(path)}: avg pieces = {avg_pieces:.2f}")
