package main

import (
	"log"
	"net"
	"os"
	"strconv"
	"time"
)

/*
func test_server(port string, c chan int, r chan int) {
	for range c{
		conn, err := net.Dial("tcp", "127.0.0.1:"+port)
		if err != nil {
			log.Println("Could not connect to server")
			return
		}
		defer conn.Close()
		b := []byte("test srting\n")
		for i := 0; i < 50; i++ {
			conn.Write(b)
		}
		r <- 1
	}
}
*/
func testServerNoThreads(port string) {
	conn, err := net.Dial("tcp", "127.0.0.1:"+port)
	if err != nil {
		log.Println("Could not connect to server")
		return
	}
	defer conn.Close()
	b := []byte("test srting\n")
	for i := 0; i < 50; i++ {
		conn.Write(b)
	}
}

func main() {
	port := os.Args[1]
	len, err := strconv.Atoi(os.Args[2])
	if err != nil {
		log.Panicln(err)
	}

	for i := 0; i < len; i++ {
		testServerNoThreads(port)
		time.Sleep(50 * time.Millisecond)
	}
	/*	workers, err := strconv.Atoi(os.Args[3])
		if err != nil{
				log.Panicln(err)
		}
		c := make(chan int, workers)
		r := make(chan int, workers)
		for i := 0; i < cap(c); i++ {
			go test_server(port, c,r)
		}
		go func(){
			for i := 0; i < len; i++ {
				c <- i
			}
		}()
		for i:=0; i< len;i++{
			log.Println(<-r)
		}
	*/
}
