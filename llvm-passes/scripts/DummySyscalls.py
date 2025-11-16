import subprocess
from pathlib import Path
import os

command = "nm -D /usr/lib/x86_64-linux-gnu/libc.so.6 | grep -E ' (T|W) ' | awk '{print $3}' | cut -d@ -f1 | sort -u"

result = subprocess.run(command, shell = True, capture_output = True, text = True)

callnames = result.stdout.splitlines()
callnames.sort()

call_map = {name: i for i, name in enumerate(callnames)}

header = []
header.append("#pragma once")
header.append("#include <string>")
header.append("#include <unordered_map>")
header.append("")
header.append("static const std::unordered_map<std::string, int> dummy_syscall_map = {")
for name in callnames:
    header.append(f"    {{\"{name}\", {call_map[name]}}},")
header.append("};")
header.append("")

script_dir = Path(__file__).parent.resolve()
out_path = script_dir.parent / "include" / "DummySyscalls.h"
out_path.parent.mkdir(parents=True, exist_ok=True)

with open(out_path, "w") as f: 
    f.write("\n".join(header))