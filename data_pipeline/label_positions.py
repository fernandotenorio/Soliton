import chess
import chess.engine
import multiprocessing as mp
import os
import csv
import time

# ---------------- CONFIG ---------------- #

ENGINE_PATH = "D:/cpp_projs/Soliton/bin/x64/Release/Soliton.exe"
ENGINE_THREADS = 1             # per engine
ENGINE_HASH_MB = 256            # per engine

SEARCH_DEPTH = 8                # fixed depth
CP_CLIP = 2000

INPUT_FEN = "demo.fen"#"../data/pool_samples/bootstrap_1M.fen"
OUTPUT_CSV = "demo_label_py.csv"#"../data/pool_labels/bootstrap_1M_label.csv"

BATCH_SIZE = 256                # positions per task
WORKERS = max(1, os.cpu_count() // 2)

# --------------------------------------- #

def clip_cp(cp):
    return max(-CP_CLIP, min(CP_CLIP, cp))

def eval_to_cp(info, turn):
    """
    Convert python-chess score to CP from side-to-move.
    """
    score = info["score"].pov(turn)

    if score.is_mate():
        return CP_CLIP if score.mate() > 0 else -CP_CLIP

    return clip_cp(score.score())

def worker_init():
    """
    Each worker creates its own engine instance.
    """
    global engine
    engine = chess.engine.SimpleEngine.popen_uci(ENGINE_PATH)
    engine.configure({
        # "Threads": ENGINE_THREADS,
        # "Hash": ENGINE_HASH_MB,
    })

def worker_eval(fen_batch):
    """
    Evaluate a batch of FENs.
    """
    results = []

    for fen in fen_batch:
        try:
            board = chess.Board(fen)
            info = engine.analyse(
                board,
                chess.engine.Limit(depth=SEARCH_DEPTH)
            )
            cp = eval_to_cp(info, board.turn)
            results.append((fen, cp))
        except Exception:
            continue

    return results

def chunked(iterable, n):
    batch = []
    for x in iterable:
        batch.append(x)
        if len(batch) == n:
            yield batch
            batch = []
    if batch:
        yield batch

# --------------------------------------- #

def main():
    t0 = time.time()

    with open(INPUT_FEN) as f:
        fens = [line.strip() for line in f if line.strip()]

    print(f"Loaded {len(fens)} positions")
    print(f"Using {WORKERS} workers")

    output_dir = os.path.dirname(OUTPUT_CSV)
    if output_dir:
        os.makedirs(os.path.dirname(OUTPUT_CSV), exist_ok=True)

    with mp.Pool(
        processes=WORKERS,
        initializer=worker_init
    ) as pool, open(OUTPUT_CSV, "w", newline="") as out:

        writer = csv.writer(out)
        writer.writerow(["fen", "cp"])

        total = 0

        for batch_result in pool.imap_unordered(
            worker_eval,
            chunked(fens, BATCH_SIZE)
        ):
            for fen, cp in batch_result:
                writer.writerow([fen, cp])
                total += 1

            if total and total % 10000 == 0:
                elapsed = time.time() - t0
                print(f"{total} labeled ({elapsed:.1f}s)")

    print(f"Done. Total labeled: {total}")
    print(f"Time: {round(time.time() - t0)}s")

if __name__ == "__main__":
    main()
