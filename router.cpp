#include <fstream>
#include <vector>
#include <set>
#include <sstream>

// Macros.
#define TRANSMISSION_DELAY(event_size, wlan_bandwidth) ((static_cast<long double>((event_size) * 8)) / static_cast<long double>(wlan_bandwidth))
#define QUEUEING_DELAY(current_time, arrival_time) ((static_cast<long double>(current_time)) - static_cast<long double>(arrival_time))
#define QUEUEING_DELAY_AVERAGE(queueing_delay, packets_out) ((static_cast<long double>(queueing_delay)) / static_cast<long double>(packets_out))

// Constants.
#define ARRIVAL 0
#define DEPARTURE 1
#define DROPPED 2

// Structs.
struct simulation_stats{
  	int buffer_space_left;
  	long double time, queuing_delay;
  	long packets_lost;
  	long packets_in, packets_out;
};

struct packet_event {
  	int category, id;
	long size;
  	double time;
};

// Lambda function for custom comparison of packet_events. If two events have equal times, then category is used for the comparison.
auto event_comparator = [](const packet_event& event_1, const packet_event& event_2) {
    return (event_1.time == event_2.time) ? (event_1.category < event_2.category) : (event_1.time < event_2.time);
};

/**
 * Prints out the fields of a simulation stats struct and the calculable QoS metrics.
 * @param stats The simulation_stats to print.
 */
void print_stats(struct simulation_stats* stats){
	printf("time = %Lf\n", (*stats).time);
	printf("packets_in = %ld\n", (*stats).packets_in);
	printf("packets_out = %ld\n", (*stats).packets_out);
	printf("packets_lost = %ld\n", (*stats).packets_lost);
	double lost_packets = ((double)(*stats).packets_lost / (double)(*stats).packets_in) * 100.0;
	printf("lost_packets = %f%\n", lost_packets);
	printf("Average queueing delay = %Lf seconds\n", QUEUEING_DELAY_AVERAGE((*stats).queuing_delay, (*stats).packets_out));
}

/**
 * Splits a string into two values with a space character as the delimiter.
 * @param input The string to split.
 * @param string_1 A variable to store the first half of the input.
 * @param string_2 A variable to store the second half of the input.
 */
void split_string(const std::string& input, std::string& string_1, std::string& string_2) {
    std::istringstream iss(input);
    iss >> string_1;
    std::getline(iss, string_2);
}

/**
 * Creates a new packet_event with the specified values.
 * @param new_id The id of the new packet_event. 
 * @param new_time The time of the new packet_event.
 * @param new_size The size of the new packet_event.
 * @param category The category of the new packet_event.
 * @return A new packet_event.
 */
packet_event new_event(int new_id, double new_time, long new_size, int category){
	struct packet_event event;
  	event.time = new_time;
  	event.size = new_size;
  	event.id = new_id;
	if(category == ARRIVAL || category == DEPARTURE || category == DROPPED){
		event.category = category;
	}
	else{
		printf("Error: Invalid packet event category. Exiting...");
		exit(1);
	}
	return event;
}

/**
 * Adds a packet_event to the simulation buffer and updates relevant statistics.
 * @param current_stats The simulation state at the time of the function call. 
 * @param event_to_add The event to add to the buffer.
 * @param buffer The buffer to add an event to.
 */
void add_buffer_event(struct simulation_stats* current_stats, packet_event* event_to_add, std::vector<packet_event>* buffer){
	(*current_stats).buffer_space_left--;
	(*buffer).push_back((*event_to_add));
}

/**
 * Removes the first packet_event from the simulation buffer and updates relevant statistics.
 * @param current_stats The simulation state at the time of the function call. 
 * @param buffer The buffer to remove the first event from.
 */
void remove_buffer_event(struct simulation_stats* current_stats, std::vector<packet_event>* buffer){
	(*buffer).erase((*buffer).begin());
  	(*current_stats).buffer_space_left++;
}

/**
 * The main function of the program.
 * @param argc The number of command line arguments passed to the program.
 * @param argv An array of command line arguments passed to the program.
 * @return 0 if the program executes successfully.
 */
int main(int argc, char *argv[]) {
	// Process command line arguments.
  	int buffer_length = std::stoi(argv[1]);
  	long wlan_bandwidth = std::stoi(argv[2]) * 1000000; // WLAN bandwidth.
  	std::vector<std::string> files;
	std::multiset<packet_event, decltype(event_comparator)> events(event_comparator);
  	for (int i = 3; i < argc; ++i) {
  	    files.push_back(argv[i]);
  	}

	// Read files.
	int packets_read = 0;
  	for(std::string file: files) {
		// Open the file.
		std::ifstream input;
  		input.open(file);

		// Read the file and populate events.
		std::string line;
  		while(std::getline(input, line)) {

			// Split the line.
  	  		struct packet_event event;
			std::string string_1;
			std::string string_2;
			split_string(line, string_1, string_2);

			// Create the new event.
  			event.category = ARRIVAL;
  			event.time = std::stod(string_1);
  			event.size = std::stol(string_2); 
  			event.id = packets_read;
  			packets_read++;
			events.insert(event);

  		}
  	}

	// Data structures for the loop.
  	std::vector<packet_event> buffer;
  	std::set<int> buffer_log;
	struct simulation_stats stats = {buffer_length, 0.0L, 0.0L, 0L, 0L, 0L};
	packet_event event;

  	while(!events.empty()) {
		// Get the next event.
  	  	event = *(events.begin());
  	  	events.erase(events.begin());

		// Deal with events by category.
  	  	if(event.category == ARRIVAL){
			// Packet received, update statistics and add to buffer if space is available.
  	  		stats.packets_in++;

  	  		if(stats.buffer_space_left <= 0) {
				// Buffer is full, mark the packet as dropped.
				events.insert(new_event(event.id, stats.time, event.size, DROPPED));
  	  		} 
			else {
				// Add the packet to the buffer.
				add_buffer_event(&stats, &event, &buffer);
  	  		}
		}
		else if(event.category == DROPPED){
			// Packet dropped, update statistics.
  	  		stats.packets_lost++;
		}
		else if(event.category == DEPARTURE){
			// Packet sent, update statistics and calculate queuing delay.
			stats.packets_out++;

  	  		// Calculate queuing delay for the packet.
  	  		long double packet_queuing_delay = QUEUEING_DELAY(stats.time, (buffer.begin()) -> time);
  	  		stats.queuing_delay += packet_queuing_delay;

  	  		// Calculate transmission delay.
			long double delay = TRANSMISSION_DELAY(event.size, wlan_bandwidth);
  	  		stats.time += delay;

  	  		// Deal with events that occurred while the packet was being sent.
  	  		while(!events.empty()) {
  	  		  	packet_event last_event = *(events.begin());

				// If the event is a departure or no event occurred.
  	  		  	if(last_event.category == DEPARTURE || last_event.time > stats.time) {
  	  		  	  	break;
  	  		  	}

  	  		  	events.erase(events.begin());
  	  		  	if(last_event.category == DROPPED) {
					// Dropped event during transmission, update statistics.
  	  		  	  	stats.packets_lost++; 
  	  		  	} 
				else { 
					// Received event during transmission, update statistics and buffer.
  	  		  	  	stats.packets_in++;

					// Buffer is full, mark the packet as dropped.
  	  		  	  	if(stats.buffer_space_left <= 0) {
						events.insert(new_event(last_event.id, stats.time, last_event.size, DROPPED));
  	  		  	  	} 
					else {
						// Add the packet to the buffer.
						add_buffer_event(&stats, &last_event, &buffer);
  	  		  	  	}
  	  		  	}
  	  		}

			// Remove the sent packet from the buffer.
			remove_buffer_event(&stats, &buffer);
		}

  	  	// Handle case where current time has no more events.
		packet_event front_event;
		if(!buffer.empty()){
			front_event = buffer.front();

			// Adjust the simulation time if necessary to match the time of the next event.
			if(!events.empty() && (stats.time < front_event.time) && (events.begin() -> time >= front_event.time)){
				stats.time = front_event.time;
			}

			// Create new departure event if needed.
			if((stats.time >= front_event.time) && (buffer_log.find(front_event.id) == buffer_log.end())) {
				buffer_log.insert(front_event.id);
				events.insert(new_event(front_event.id, stats.time, front_event.size, DEPARTURE));
  	  		}
		}
  	}

  	// Print final simulation stats.
	print_stats(&stats);

  	return 0;
}
