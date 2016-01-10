for f in ../genome-filtered/*.fa; do
	echo $f
	./test.sh $f
done
