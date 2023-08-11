package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"log"
	"os"
	"strings"
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

	log.SetFlags(0)

	var input []byte
	if input, err = hex.DecodeString(strings.Join(os.Args[1:], "")); err != nil {
		return
	}

	var ent []byte
	{
		sum := sha256.Sum256(input)
		ent = append(ent, sum[:]...)

		sum = sha256.Sum256(ent)
		ent = append(ent, sum[0])
	}

	out := &bytes.Buffer{}

	for _, i := range ent {
		out.WriteString(fmt.Sprintf("%08b", i))
	}

	i := 0

	for {
		i++

		if buf := out.Next(11); len(buf) < 11 {
			break
		} else {
			log.Printf("%02d.%s", i, buf)
		}
	}
}
