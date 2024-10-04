package main
import (
  "os"
  "flag"
  "encoding/binary"
  "fmt"
)

func main() {
  var nFlag = flag.Uint("n", 4096, "How many lines")
  var isBinary = flag.Bool("b", false, "Ouput Binary")
  flag.Parse();
  if(*isBinary) {
    b := make([]byte, 8)
    for i := uint64(0); i < uint64(*nFlag); i++ {
      binary.LittleEndian.PutUint64(b, uint64(i));
      os.Stdout.Write(b[:]);
    }
  } else {
    for i := uint64(0); i < uint64(*nFlag); i++ {
      var fmtNewLine = "%d\n";
      var fmtTab = "%d\t";
      var chosenFmt string;
      if i % 2 == 1 {
        chosenFmt = fmtNewLine;
      } else {
        chosenFmt = fmtTab;
      }
      fmt.Printf(chosenFmt, i);
    }
  }
}
