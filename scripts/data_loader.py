from pathlib import Path
from pm4py import ProcessTree, discover_process_tree_inductive
from pm4py.objects.log.obj import EventLog
from pm4py.objects.process_tree.utils.generic import get_leaves
import pandas as pd
import pm4py

def discover_process_tree(
    event_log: EventLog | pd.DataFrame, noise_threshold: float = 0.25
) -> ProcessTree:
    # Discover process tree using Inductive Miner infrequent
    return discover_process_tree_inductive(event_log, noise_threshold=noise_threshold)


xesPath = Path("data/xes").resolve()
ptmlPath = Path("data/ptml").resolve()

for xes_file in xesPath.glob("*.xes"):
    if not xes_file.is_file():
        continue

    event_log = pm4py.read_xes(str(xes_file))

    # create ptml files if corresponding ptml doesn't exist..
    if not (ptml_file := ptmlPath / f"{xes_file.stem}.ptml").is_file():
        
        ptmlVariants = [
            (0.0, "_pt00"),
            (0.1, "_pt10"),
            (0.25, "_pt25"),
            (0.5, "_pt50"),
        ]
        for noise_threshold, file_tag in ptmlVariants:
            if (ptml_file := ptmlPath / f"{xes_file.stem}{file_tag}.ptml").is_file():
                continue

            process_tree = discover_process_tree(
                event_log, noise_threshold=noise_threshold
            )
            pm4py.write_ptml(
                process_tree, str(ptmlPath / f"{xes_file.stem}{file_tag}.ptml")
            )
    