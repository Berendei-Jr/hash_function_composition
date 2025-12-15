import hashlib
from pathlib import Path
from typing import Dict
import sys

# {
#   "<var_name>": <var_value>,
# }
variables: Dict[str, bytes] = {}


def get_var(var_with_brackets):
    # (var-0) -> var-0
    var_with_brackets = var_with_brackets.strip()
    return var_with_brackets.strip("(").strip(")")


if __name__ == "__main__":
    with open(Path(__file__).parent / "order.txt", "r") as f:
        order = f.readlines()

    target_hash = bytes.fromhex(
        "a074f354f01eedaf5d26720b73d60a232e1b77071ec1a8aef93e84c348c8f6089928c03fff260bdd35a631c4b4a0fbee488a8ddd3169e54cc969a03e154db4ec"
    )

    i = 0

    while i < 1000000001:
        for line in order:
            if line.startswith("Input"):
                init_var = get_var(line.split(",")[-1])
                variables[init_var] = i.to_bytes(4, "little")
            elif line.startswith("Result"):
                result_var = get_var(line.split(",")[-1])
                # print(f"Результат: {variables[result_var]}")
                if variables[result_var] == target_hash:
                    print(f"Успешно! Искомое число {i}")
                    sys.exit(0)
                i += 1
            elif line.startswith("Md5"):
                md5_input_var = get_var(line.split(",")[1])
                md5_result_var = get_var(line.split(",")[-1])
                md5 = hashlib.md5(variables[md5_input_var]).digest()
                variables[md5_result_var] = md5

            elif line.startswith("Sha-512"):
                sha512_input_var = get_var(line.split(",")[1])
                sha512_result_var = get_var(line.split(",")[-1])
                sha512 = hashlib.sha512(variables[sha512_input_var]).digest()
                variables[sha512_result_var] = sha512

            elif line.startswith("Xor"):
                xor_input_vars = get_var(line.split(",")[1])
                xor_input_1, xor_input_2 = xor_input_vars.split(" ")
                xor_input_1_value = variables[xor_input_1]
                xor_input_2_value = variables[xor_input_2]

                result_length = max(len(xor_input_1_value), len(xor_input_2_value))
                xor_input_1_value = xor_input_1_value.ljust(result_length, b"\x00")
                xor_input_2_value = xor_input_2_value.ljust(result_length, b"\x00")

                xor_result_var = get_var(line.split(",")[-1])

                xor = bytes(a ^ b for a, b in zip(xor_input_1_value, xor_input_2_value))

                variables[xor_result_var] = xor
