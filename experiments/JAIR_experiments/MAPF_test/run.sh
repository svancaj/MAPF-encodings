for task in tasks/*
do
	qsub $task
done