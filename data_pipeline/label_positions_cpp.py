import os
import sys
import subprocess
import multiprocessing
import time
import glob

# ---------------- CONFIGURATION ---------------- #

# Path to your compiled C++ engine
ENGINE_PATH = "D:/cpp_projs/Soliton/bin/x64/Release/Soliton.exe"

# Input/Output paths
INPUT_FEN_FILE = "demo.fen"#"../data/pool_samples/bootstrap_1M.fen"
OUTPUT_CSV_FILE = "demo_label.csv"#"../data/pool_labels/bootstrap_1M_labeled.csv"
TEMP_DIR = "temp_processing"

# Search parameters
DEPTH = 8

# ----------------------------------------------- #

def split_input_file(input_path, num_chunks, output_dir):
    """
    Splits the large input file into smaller chunk files.
    Returns a list of filenames.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(input_path, 'r') as f:
        lines = [line.strip() for line in f if line.strip()]

    total_lines = len(lines)
    chunk_size = (total_lines // num_chunks) + 1
    
    chunk_files = []

    print(f"Splitting {total_lines} positions into {num_chunks} chunks...")

    for i in range(num_chunks):
        start = i * chunk_size
        end = min((i + 1) * chunk_size, total_lines)
        
        if start >= total_lines:
            break

        chunk_filename = os.path.abspath(os.path.join(output_dir, f"chunk_{i}.fen"))
        with open(chunk_filename, 'w') as f_out:
            f_out.write("\n".join(lines[start:end]))
        
        chunk_files.append(chunk_filename)

    return chunk_files


def run_worker(args):
    """
    Worker function to call the C++ engine.
    """
    chunk_file, output_file, engine_path, depth = args
    
    # Updated Command: eval <input> <output> <depth>
    # Note: We wrap paths in quotes in case they have spaces, though simple paths are safer.
    cmd_input = f"eval {chunk_file} {output_file} {depth}\nquit\n"

    try:
        # 1. Start process
        # We don't need to capture stdout/stderr anymore, but it's good to keep them 
        # to prevent buffer blocking if the engine prints debug info.
        proc = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL, # Ignore standard output
            stderr=subprocess.PIPE,    # Capture errors if any
            text=True,
            cwd=os.path.dirname(engine_path) 
        )

        # 2. Send command
        _, stderr_data = proc.communicate(input=cmd_input)

        if stderr_data:
            print(f"Engine Error on {chunk_file}: {stderr_data}")

        # 3. Validation
        # Check if output file was created
        if os.path.exists(output_file):
            # Return number of lines (optional, purely for stats)
            with open(output_file, 'rb') as f:
                return sum(1 for _ in f)
        return 0

    except Exception as e:
        print(f"Worker failed on {chunk_file}: {e}")
        return 0

def main():
    t0 = time.time()
    
    # 1. Determine cores
    total_cores = os.cpu_count() or 1
    workers = max(1, total_cores // 2)
    print(f"Using {workers} workers (50% of {total_cores} cores)")

    # 2. Split Data
    chunk_inputs = split_input_file(INPUT_FEN_FILE, workers, TEMP_DIR)
    
    # 3. Prepare worker arguments
    # List of tuples: (input_chunk_path, output_chunk_path, engine_path, depth)
    tasks = []
    output_chunks = []
    
    for i, input_chunk in enumerate(chunk_inputs):
        output_chunk = os.path.abspath(os.path.join(TEMP_DIR, f"result_{i}.csv"))
        output_chunks.append(output_chunk)
        tasks.append((input_chunk, output_chunk, ENGINE_PATH, DEPTH))

    # 4. Run in Parallel
    print("Starting engine processes...")
    total_labeled = 0
    
    with multiprocessing.Pool(processes=workers) as pool:
        results = pool.map(run_worker, tasks)
        total_labeled = sum(results)

    # 5. Merge Results
    print(f"Merging results into {OUTPUT_CSV_FILE}...")
    
    output_dir = os.path.dirname(OUTPUT_CSV_FILE)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    
    with open(OUTPUT_CSV_FILE, 'w') as outfile:
        for partial_file in output_chunks:
            if os.path.exists(partial_file):
                with open(partial_file, 'r') as infile:
                    outfile.write(infile.read())

    # 6. Cleanup
    try:
        import shutil
        shutil.rmtree(TEMP_DIR)
    except Exception as e:
        print(f"Warning: Could not remove temp dir: {e}")

    t1 = time.time()
    print("-" * 40)
    print(f"Done.")
    print(f"Positions Labeled: {total_labeled}")
    print(f"Total Time: {round(t1 - t0)}s")

if __name__ == "__main__":
    # Windows support for multiprocessing
    multiprocessing.freeze_support()
    main()