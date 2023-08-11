package main

import (
	"crypto/rand"
	"crypto/sha256"
	"log"
	"os"
	"time"
)

func main() {
	var err error
	defer func() {
		if err == nil {
			return
		}
		log.Println("exited with error:", err.Error())
		os.Exit(1)
	}()

	log.SetFlags(log.Ltime | log.Lmicroseconds)

	var (
		buf = make([]byte, 512)
		sum [sha256.Size224]byte

		vals = []string{"UP", "DOWN", "LEFT", "RIGHT"}
	)

	for {
		if _, err = rand.Read(buf); err != nil {
			return
		}

		sum = sha256.Sum224(buf)
		v, d := sum[0], sum[1]

		log.Println(vals[int(v)%len(vals)])

		time.Sleep(time.Second + time.Millisecond*200*time.Duration(d%16))
	}
}
