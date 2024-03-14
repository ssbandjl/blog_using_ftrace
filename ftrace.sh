# cat /sys/kernel/debug/tracing/events/sched/
# find /sys/kernel/debug/tracing/events/ -type d
# perf list 2>/dev/null

cd /sys/kernel/debug/tracing
echo 0 > tracing_on
echo function_graph > current_tracer
#echo function > current_tracer
echo itdev_example_cdev_read_special_data > set_graph_function
#echo itdev_example_cdev_read_special_data > set_ftrace_filter
# echo > set_ftrace_filter
echo 1 > options/func_stack_trace
echo 1 > tracing_on
sleep 3
cat trace

