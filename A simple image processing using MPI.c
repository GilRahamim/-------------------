#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

int* read_pgm(const char* filename, int* width, int* height, char* header, int header_size){
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fgets(header, header_size, file); // Read the header

    chaer line[100];
    fgets(line, sizeof(line), file); // Read the width and height
    while (line[0] == '#') {
        strcat(header, line); // Append comments to header
        fgets(line, sizeof(line), file); // Skip comments
    }

    strcat(header, line); // Append width and height to header
    sscanf(line, "%d %d", width, height); // Extract width and height

    fgets(line, sizeof(line), file); // Read the max value
    strcat(header, line); // Append max value to header


    int* image = (int*)malloc((*width) * (*height) * sizeof(int));
    if (!image) {
        perror("Error allocating memory for image");
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < (*width) * (*height); i++) {
        fscanf(file, "%d", &image[i]); // Read pixel values
    }

    fclose(file);
    return image;
}

void write_pgm(const char* filename, int* image, int width, int height, const char* header){
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }

    fprintf(file, "%s", header); // Write the header

    for (int i = 0; i < width * height; i++) {
        fprintf(file, "%d\n", image[i]); // Write pixel values
    }

    fclose(file);
}

int main(int argc, char** argv){
    int rank, size;
    double start_time, end_time;
    char input_file[256], output_file[256];
    int image* = NULL, *local_image = NULL, *result_image = NULL;
    int width, height, local_height, remander;
    char header[1024] = {0};

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    strcpy(input_file, argv[1]);
    strcpy(output_file, argv[2]);

    start_time = MPI_Wtime();

    if (rank == 0) {
        image = read_pgm(input_file, &width, &height, header, sizeof(header));
        if (!image) {
            printf("Error reading image\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        printf("Image size: %d x %d\n", width, height);

    }

    mpi_bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    mpi_bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    local_height = height / size;
    remander = height % size;

    if (rank == size - 1) {
        local_height += remainder;
    }


    local_image = (int*)malloc(width * local_height * sizeof(int));
    if (!local_image) {
        printf("Memory allocation failed for local image in process %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }


        // חלוקת התמונה בין התהליכים
    int* sendcounts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));
        
    for (int i = 0; i < size; i++) {
        sendcounts[i] = width * (height / size);
        if (i == size - 1) {
            sendcounts[i] += width * remainder;
        }
        displs[i] = i * width * (height / size);
    }

    MPI_Scatterv(image, sendcounts, displs, MPI_INT, local_image, 
        width * local_height, MPI_INT, 0, MPI_COMM_WORLD);
        
    for (int i = 0; i < width * local_height; i++) {
        local_image[i] = local_image[i] / 2;
    }


    if (rank == 0) {
        result_image = (int*)malloc(width * height * sizeof(int));
        if (!result_image) {
            printf("Memory allocation failed for result image in process %d\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }


    MPI_Gatherv(local_image, width * local_height, MPI_INT, result_image, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        write_pgm(output_file, result_image, width, height, header);
        printf("Processed image saved to %s\n", output_file);
    }

    end_time = MPI_Wtime();
    
    if (rank == 0) {
        printf("Processing time: %f seconds\n", end_time - start_time);
    }
    
    // שחרור זיכרון
    free(local_image);
    if (rank == 0) {
        free(image);
        free(result_image);
    }
    free(sendcounts);
    free(displs);
    
    MPI_Finalize();
    return 0;
}