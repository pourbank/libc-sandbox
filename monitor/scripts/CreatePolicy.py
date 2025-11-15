import struct
import re
import sys

EDGE_RE = re.compile(r'(\d+)\s*->\s*(\d+)\s*\[label="(\d+)"\];')
NODE_RE = re.compile(r'(\d+)\s*\[.*\];')

dot_file_path = "../llvm-passes/output/Syscall.dot"
bin_file_path = "policy.bin"

with open(dot_file_path, 'r') as f:
    content = f.read()

transitions = []
from_nodes = set()
all_nodes = set()
start_node = 0

for (node_s) in NODE_RE.findall(content):
    all_nodes.add(int(node_s))

for (from_s, to_s, label_s) in EDGE_RE.findall(content):
    from_n = int(from_s)
    to_n = int(to_s)
    label_n = int(label_s)
    
    transitions.append((from_n, to_n, label_n))
    
    if from_n != to_n:
        from_nodes.add(from_n)

    all_nodes.add(from_n)
    all_nodes.add(to_n)

final_states = all_nodes - from_nodes

num_nodes = max(all_nodes) + 1
num_transitions = len(transitions)
num_final_states = len(final_states)
final_states_list = sorted(list(final_states))

try:
    with open(bin_file_path, 'wb') as f:
        header_format = 'iiii' 
        header_data = struct.pack(header_format, 
                                  num_nodes, 
                                  start_node, 
                                  num_final_states, 
                                  num_transitions)
        f.write(header_data)

        if num_final_states > 0:
            final_states_format = f'{num_final_states}i'
            final_states_data = struct.pack(final_states_format, *final_states_list)
            f.write(final_states_data)

        transition_format = 'iii'
        for (from_n, to_n, label_n) in transitions:
            transition_data = struct.pack(transition_format, from_n, to_n, label_n)
            f.write(transition_data)

except Exception as e: 
    sys.exit(1)