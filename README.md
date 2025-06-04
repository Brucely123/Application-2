

Engineering Analysis (Discussion 
Task Timing and Jitter: Compare the timing of the LED blink and console print tasks with that of the sensor task. How regular is each task’s period in practice? **Highly regular. Sensor alert messages ( "Eclipse Warning: Avg Solar Lux = ") appear every 500 ms consistently.**
 For the sensor task (using vTaskDelayUntil), do the sensor readings and alert messages occur at consistent intervals (e.g. every 500 ms)?
 **Due to the fact that vTaskDelayUntil(&lastWakeTime, periodTicks) always adds an offset (periodTicks) to lastWakeTime, not to the tick count of the current task. The 500 ms window allows each wake-up to occur at exactly 500 ms intervals, regardless of how long a task takes to complete.**


 In contrast, do you observe any drift or variation in the LED blink or print intervals over time? **Despite slight variations in execution, timing realigns automatically on the next cycle.**




Explain why vTaskDelayUntil provides a more stable period for the sensor task, referencing how it calculates the next wake-up tick based on an absolute time reference. What might cause jitter in the LED or print task periods when using vTaskDelay? (Hint: consider the effect of the sensor task running at the moment they are ready to run, and how that might delay them slightly.) 
**With vTaskDelayUntil(), you can ensure the timing of tasks is stable, precise, even if task execution varies. Due to preemption and execution delays caused by vTaskDelay(), timing can drift over time, causing jitter, especially for higher-priority tasks (like sensor tasks).**


Priority-Based Preemption: Describe a scenario observed in your running system that demonstrates FreeRTOS’s priority-based preemptive scheduling. For example, what happens if the console print task is about to print (or even mid-way through printing) exactly when the sensor task’s next period arrives? 
**As the console print task (low priority) prints its heartbeat message in the running system, FreeRTOS's priority-based preemptive scheduling is demonstrated when, at the same time, the high priority sensor task reaches its scheduled period and is set to begin. It can be observed in the simulation output when a sensor reading appears earlier than expected, even though the printing was expected at the same time. As a result, the lower-priority print task was preempted by the higher-priority sensor task, confirming FreeRTOS's behavior.**
Does the sensor task interrupt the print task immediately, or does it wait?
**As soon as the sensor task becomes available (unblocked) and the console print task is running or about to run, the sensor task interrupts the print task. **
 Based on your understanding of FreeRTOS, which task would the scheduler choose to run at a moment when both become Ready, and why?
**According to my understanding of FreeRTOS, the scheduler will run the task with the highest priority when two tasks become Ready at the same time. It has three priorities: Sensor, LED, and print. Sensor has a priority of 3, LED has a priority 2, and print has a priority 1. Due to its higher priority, the scheduler will select the sensor task over the print task if they both become Ready at the same time.**
 Provide a brief timeline or example (using tick counts or event ordering) to illustrate the preemption. (If you didn’t explicitly catch this in simulation, answer conceptually: assume the print task was running right when the sensor task unblocked – what should happen?)
**In order to illustrate the concept of preemption, the following example could be used: At tick 5000, the console print task is executed and a heartbeat message is output. As a result of its scheduled 500 ms delay expiring, the sensor task also becomes ready at the same time. Due to the sensor task's higher priority, FreeRTOS preempts the print task immediately and switches execution to the sensor task. By using vTaskDelayUntil, the sensor task completes its lux measurement and moves into a blocked state. Once this has been completed, FreeRTOS resumes the print task. In this example, we can see how the sensor task interrupts the print task to maintain a precise timing and demonstrate true preemptive scheduling.**
Effect of Task Execution Time: In our design, all tasks have small execution times (they do minimal work before blocking again). Suppose the sensor task took significantly longer to execute (for instance, imagine it performed a complex calculation taking, say, 300 ms of CPU time per cycle). How would that affect the lower-priority tasks?
**This increased execution time can delay the scheduling of lower-priority tasks such as LED blinks and console prints if the sensor task takes a significant period of time to execute. The priority-based preemptive scheduling in FreeRTOS causes the high-priority sensor task to dominate the CPU while it is running, delaying the execution of lower-priority tasks. **


 Discuss what would happen if the sensor task’s execution time sometimes exceeds its period (i.e., it can’t finish its work before the next 500 ms tick). What symptoms would you expect to see in the system (e.g. missed readings, delayed LED toggles, etc.)?
**The sensor task would miss its intended start time for the next sampling cycle if it exceeded its 500 ms execution period occasionally ( taking 600 ms). This results in missed readings and possibly back-to-back executions, which further starves other tasks. This might be manifested by delayed or skipped LED toggles, an erratic or clumped sensor print output, or inconsistent heartbeat (telemetry) messages. Such overloads can be identified by the jitter in LED blink rates, irregular heartbeat counts in the serial console, and possibly irregular sensor readings.**


 Relate this to real-time scheduling concepts like missed deadlines or CPU utilization from the RTOS theory (Chapters 3 and 6 of the Harder textbook). What options could a system designer consider if the high-priority task started starving lower tasks or missing its schedule (think about reducing workload, adjusting priorities, or using two cores)?
**There is a missed deadline here, as a periodic task (the sensor) has not been completed before it is time for it to be activated again. Furthermore, the total execution load may exceed what the processor can handle within fixed time windows, raising concerns about CPU utilization. An RTOS is able to schedule tasks as long as CPU utilization does not exceed a certain threshold. If it exceeds this threshold, it risks unresponsiveness and instability.**


vTaskDelay vs vTaskDelayUntil: Why did we choose vTaskDelayUntil for the sensor task instead of using vTaskDelay in a simple loop? Explain in your own words the difference between these two delay functions in FreeRTOS, and the specific problem that vTaskDelayUntil solves for periodic real-time tasks. Consider what could happen to the sensor sampling timing over many iterations if we used vTaskDelay(500 ms) instead – how might small errors accumulate?
**As part of the sensor task, we use vTaskDelayUntil since it references an absolute tick count, not a relative delay, to ensure a fixed and consistent sampling interval. When vTaskDelay is used, the task will be paused for a fixed period of time relative to the end of the previous task execution, allowing it to be run longer or get preempted if needed. Due to this, the sensor runs at inconsistent or increasingly delayed intervals over many cycles, leading to timing drift. It is crucial for real-time systems, especially periodic tasks like sensor sampling, to maintain a steady execution rate, which vTaskDelayUntil effectively provides.**
 Also, for the LED blink task, why is using vTaskDelay acceptable in that context? (Think about the consequences of slight timing drift for a status LED vs. a sensor sampling task.)
**Status LEDs do not require precise timing, so vTaskDelay can be employed. Since the LED primarily serves as a visual indicator, if the blink interval drifts slightly (for instance, blinking at 502 ms instead of 500 ms), this won't affect the system's correctness or user perception. As the LED blink task is not time-sensitive or event-driven, vTaskDelay provides a simpler, more sufficient way of implementing periodic blinks.**


Thematic Integration Reflection: Relate the functioning of your three tasks to a real-world scenario in one of the thematic contexts (Space Systems, Healthcare, or Hardware Security). Describe an example of what each task could represent. Explain how task priority might reflect the importance of that function in the real system (e.g. why the sensor monitoring is high priority in a medical device).
**A space system's sensor task represents solar monitoring and is given the highest priority for power and safety reasons. In a medium priority system, LED blinking acts as a visual status indicator. It is the lowest priority task, as console print simulates telemetry reporting. In order to guarantee reliable monitoring, critical monitoring tasks are prioritized, while less crucial tasks are scheduled based on resource availability.**


Bonus - design an experiment which causes starvation in the system (e.g., sensor task never gives time to the other tasks). Describe the code you used and the results. Leave the code in your environment but comment it out. Return your code for submission to a working ideal state.
**I modified sensor_task() by commenting out vTaskDelayUntil() at the end of its loop to demonstrate task starvation. Consequently, the sensor task ran continuously without ever relinquishing control to the lower-priority tasks. Consequently, the LED blink task and console print task were completely starved of CPU time. As a result of an unblocked high-priority task, only the sensor's LUX readings appeared rapidly in the simulation output, while the LED and telemetry messages ceased to blink, confirming that starvation occurred.**

