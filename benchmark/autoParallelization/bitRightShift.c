// Test the handling of >> operator
#define  TOTAL_KEYS_LOG_2    16
#define  NUM_BUCKETS_LOG_2   9
#define  TOTAL_KEYS          (1 << TOTAL_KEYS_LOG_2)
#define  NUM_KEYS            TOTAL_KEYS
#define  SIZE_OF_BUFFERS     NUM_KEYS 
#define  NUM_BUCKETS         (1 << NUM_BUCKETS_LOG_2)

int key_array[SIZE_OF_BUFFERS]; // 1<< 9
int bucket_size[NUM_BUCKETS];  // 1<<9
void foo(int shift)
{
  int i; 
  for(i=0; i<NUM_KEYS; i++ ) // 1<<9
    bucket_size[key_array[i] >> shift]++;
}
