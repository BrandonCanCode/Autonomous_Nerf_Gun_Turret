import sysv_ipc
import struct
import argparse

# Generate the same key value as in the C process
key = sysv_ipc.ftok("/tmp", 42)  # Use an integer value for proj_id

# Access the same shared memory segment
shm = sysv_ipc.SharedMemory(key, flags=sysv_ipc.IPC_CREAT, size=5 * struct.calcsize("III"))  # Assuming each target_info struct is 12 bytes (3 * uint32_t)

def update_target_info(target, x, y):
    # Read all target_info structs from shared memory
    target_infos_bytes = shm.read(5 * struct.calcsize("III"))

    # Unpack target_info structs
    target_infos = [struct.unpack("III", target_infos_bytes[i*12:(i+1)*12]) for i in range(5)]
    
    # Update the target_info struct with the specified target number
    if target >= 0 and target < 5:
        # Ensure X doesn't surpass 640 and Y doesn't surpass 420
        x = min(x, 640)
        y = min(y, 420)

        # Update the specified target_info struct
        target_infos[target] = (target, x, y)

        # Pack the updated target_info structs
        new_target_infos_bytes = b"".join(struct.pack("III", *target_info) for target_info in target_infos)

        # Write the updated target_info structs to shared memory
        shm.write(new_target_infos_bytes)
        print(f"Updated target {target}: x={x}, y={y}")
    else:
        print("Invalid target number. Target number must be between 0 and 4.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Update target_info values in shared memory")
    parser.add_argument("target", type=int, help="Target number (0-4)")
    parser.add_argument("x", type=int, help="X coordinate (0-640)")
    parser.add_argument("y", type=int, help="Y coordinate (0-420)")
    args = parser.parse_args()

    update_target_info(args.target, args.x, args.y)
