import chess
import random
import time
import os
import multiprocessing
from functools import partial

# ---------------- CONFIGURATION & CONSTANTS ---------------- #

# Pawn on 7th rank filter constants
PROMO_RANK_WHITE = chess.BB_RANK_7
PROMO_RANK_BLACK = chess.BB_RANK_2

# Filter params
HALF_MOVE_CLOCK = 90
FULL_MOVE_NUMBER = 80
SAMPLE_RATE = 6
MIN_PIECES = 12

def pawn_on_7th(board):
    if board.turn == chess.WHITE:
        return bool(board.pieces(chess.PAWN, chess.WHITE) & PROMO_RANK_WHITE)
    else:
        return bool(board.pieces(chess.PAWN, chess.BLACK) & PROMO_RANK_BLACK)

# ---------------- WORKER FUNCTION ---------------- #

def _process_single_file(file_pair_seed):
    """
    Worker function to process a single file.
    file_pair_seed: triple (input_path, output_path, seed)
    Returns: tuple (seen_count, kept_count, processing_time)
    """
    input_file, output_file, seed = file_pair_seed
    
    # Ensure distinct random seeds for each process
    random.seed(seed) 
    
    seen = 0
    kept = 0
    already_seen = set()
    t_start = time.time()

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    try:
        with open(input_file, "r") as fin, open(output_file, "w") as fout:
            for line in fin:
                line = line.strip()

                if not line:
                    continue

                seen += 1

                # Random sampling
                if random.randrange(SAMPLE_RATE) != 0:
                    continue

                # Deduplication (Per file context)
                if line in already_seen:
                    continue

                already_seen.add(line)

                try:
                    board = chess.Board(line)
                except Exception:
                    continue

                # ---------------- FILTERS ---------------- #

                if len(board.piece_map()) < MIN_PIECES:
                    continue

                if board.is_check():
                    continue

                if pawn_on_7th(board):
                    continue

                if board.is_insufficient_material():
                    continue

                if board.halfmove_clock >= HALF_MOVE_CLOCK:
                    continue

                if board.fullmove_number > FULL_MOVE_NUMBER:
                    continue

                fout.write(line + "\n")
                kept += 1
                
    except Exception as e:
        print(f"Error processing {input_file}: {e}")
        return 0, 0, 0

    t_end = time.time()
    return seen, kept, (t_end - t_start)

# ---------------- MAIN PROCESS FUNCTION ---------------- #

def process_fens_parallel(file_mapping_seed):
    """
    Processes FEN files in parallel using 50% of available cores.
    
    Args:
        file_mapping_seed (dict): A list of triples [('input_file_path', 'output_file_path', seed), ..., ]
    """
    
    # Determine CPU usage (50% of cores, minimum 1)
    total_cores = os.cpu_count() or 1
    workers = max(1, total_cores // 2)
    
    print(f"Starting processing on {workers} cores (Total detected: {total_cores})...")
    
    # Prepare list of arguments for the worker
    tasks = file_mapping_seed
    
    t_global_start = time.time()
    
    total_seen = 0
    total_kept = 0
    
    # Initialize Pool
    with multiprocessing.Pool(processes=workers) as pool:
        # imap_unordered is often more efficient for simple tasks as it yields results as they finish
        results = pool.imap_unordered(_process_single_file, tasks)
        
        # Collect results
        for i, (seen, kept, duration) in enumerate(results, 1):
            total_seen += seen
            total_kept += kept
            print(f"[{i}/{len(tasks)}] Finished file. Kept: {kept}/{seen} ({duration:.2f}s)")

    t_global_end = time.time()
    
    print("-" * 40)
    print(f"Total Seen positions: {total_seen}")
    print(f"Total Kept positions: {total_kept}")
    print(f"Global Retention ratio: {total_kept / max(1, total_seen):.3f}")
    print(f"Total Time: {round(t_global_end - t_global_start)}s")
