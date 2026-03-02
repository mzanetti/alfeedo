import os
import gzip
import shutil
from SCons.Script import Import

Import("env")

def gzip_requested_assets(source, target, env):
    config = env.GetProjectConfig()
    section = "env:" + env["PIOENV"]

    embed_files = config.get(section, "board_build.embed_txtfiles", "")

    # Ensure it's a list (PIO can be inconsistent here)
    if isinstance(embed_files, str):
        embed_files = embed_files.split()

    print("\n--- Processing embed_txtfiles ---")

    for file_path in embed_files:
        # We only care about files ending in .gz
        if file_path.endswith('.gz'):
            # The source is the same path minus the .gz extension
            source_file = file_path[:-3] 
            full_source = os.path.join(env.get("PROJECT_DIR"), source_file)
            full_target = os.path.join(env.get("PROJECT_DIR"), file_path)

            if os.path.exists(full_source):
                # Check if we actually need to re-zip (timestamp check)
                if not os.path.exists(full_target) or os.path.getmtime(full_source) > os.path.getmtime(full_target):
                    with open(full_source, 'rb') as f_in:
                        with gzip.open(full_target, 'wb', compresslevel=9) as f_out:
                            shutil.copyfileobj(f_in, f_out)
                    print(f"  [GZIPPED] {source_file} -> {file_path}")
            else:
                print(f"  [WARNING] Source for {file_path} not found at {source_file}")

# Execute immediately to ensure files exist before the linker looks for them
gzip_requested_assets(None, None, env)
