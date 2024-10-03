package main
import (
  "os"
  "flag"
  "encoding/binary"
)

func main() {
  var nFlag = flag.Uint("n", 4096, "How many lines")
  flag.Parse();
  b := make([]byte, 8)
  for i := uint64(0); i < uint64(*nFlag); i++ {
    binary.LittleEndian.PutUint64(b, uint64(i));
    os.Stdout.Write(b[:]);
  }
}
