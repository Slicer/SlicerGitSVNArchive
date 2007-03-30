LOOPY=../../loopy --retries 10
SEGMENTER=./randomFail --p-fail 0.0 --delay 2000
STATISTICS=./randomFail --p-fail 0.0 --delay 2000


all: data/summary-statistics.out
	@echo job complete

data/summary-statistics.out: data/ex1.out data/ex2.out
	@echo making summary
	$(LOOPY) -d data/summary-statistics.out "$(STATISTICS) -o data/summary-statistics.out"

data/ex1.out:
	@echo making ex1
	$(LOOPY) -d data/ex1.out "$(SEGMENTER) -o data/ex1.out"
data/ex2.out:
	@echo making ex2
	$(LOOPY) -d data/ex2.out "$(SEGMENTER) -o data/ex2.out"

clean:
	rm -f data/ex1.out data/ex2.out data/summary-statistics.out
