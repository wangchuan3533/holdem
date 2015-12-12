all: test_hand test_client

test_hand: test_hand.c ../card.c ../hand.c 
	cc -I.. -o $@ $^

test_client: test_client.c
	cc -o $@ $^ -levent

clean:
	rm -rf *.o test_hand test_client
