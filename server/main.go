// AIKON-VXP test server - chunk 1 counterpart.
//
// Deliberately dumb: plain HTTP (no TLS yet), one endpoint, plain text
// reply. This exists purely so NetTest.vxp has something real to talk
// to and prove the phone-to-server round trip works at all. TLS 1.0
// (what the Nokia can actually negotiate) is a separate, later chunk -
// see AIKON's own server/ for how that was done for Series 40 phones.
package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"time"
)

func pingHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain; charset=us-ascii")
	fmt.Fprintf(w, "pong from AIKON-VXP server, %s\n", time.Now().UTC().Format(time.RFC3339))
	log.Printf("ping from %s", r.RemoteAddr)
}

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	http.HandleFunc("/ping", pingHandler)
	addr := ":" + port
	log.Printf("listening plain HTTP on %s", addr)
	if err := http.ListenAndServe(addr, nil); err != nil {
		log.Fatal(err)
	}
}
