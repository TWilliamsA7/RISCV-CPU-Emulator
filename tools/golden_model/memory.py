# memory.py

class Memory:
    def __init__(self, size):
        self.mem = bytearray(size)


    def loadBinary(self, file_path: str):    
        try:
            with open(file_path, 'rb') as file:
                binary_data = file.read()
                self.mem = bytearray(binary_data)
        except FileNotFoundError:
            print(f"Error: The file '{file_path}' was not found.")
        except Exception as e:
            print(f"An error occurred: {e}")


    def read8(self, addr):
        return self.mem[addr]


    def read16(self, addr):
        return int.from_bytes(self.mem[addr:addr+2], "little")


    def read32(self, addr):
        return int.from_bytes(self.mem[addr:addr+4], "little")


    def write8(self, addr, value):
        self.mem[addr] = value & 0xFF


    def write16(self, addr, value):
        self.mem[addr:addr+2] = (value & 0xFFFF).to_bytes(2, "little")


    def write32(self, addr, value):
        self.mem[addr:addr+4] = (value & 0xFFFFFFFF).to_bytes(4, "little")
