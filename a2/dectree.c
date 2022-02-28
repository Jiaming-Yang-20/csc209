/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Mustafa Quraish, Bianca Schroeder, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2021 Karen Reid
 */

#include "dectree.h"


/**
 * Load the binary file, filename into a Dataset and return a pointer to 
 * the Dataset. The binary file format is as follows:
 *
 *     -   4 bytes : `N`: Number of images / labels in the file
 *     -   1 byte  : Image 1 label
 *     - NUM_PIXELS bytes : Image 1 data (WIDTHxWIDTH)
 *          ...
 *     -   1 byte  : Image N label
 *     - NUM_PIXELS bytes : Image N data (WIDTHxWIDTH)
 *
 * You can set the `sx` and `sy` values for all the images to WIDTH. 
 * Use the NUM_PIXELS and WIDTH constants defined in dectree.h
 */
Dataset *load_dataset(const char *filename) {
    // TODO: Allocate data, read image data / labels, return
    FILE *data_file;
    data_file  = fopen(filename, "rb");
    if (data_file == NULL){
        fprintf(stderr, "Error: could not open file\n");
        exit(1);

    }
    int num_image;
    // read num_image from the file + error checking
    if(fread(&num_image, sizeof(int), 1, data_file)== 0){
        if(feof(data_file)){
            fprintf(stderr,"End of file\n");
        }else{
            perror("fread");
            exit(1);
        }
    }

    Dataset *dataset = malloc(sizeof(Dataset));//since something is very very wrong if malloc fails we often leave out the check.  
    dataset->num_items = num_image;
    dataset->images = malloc(sizeof(Image)*num_image);
    dataset->labels = malloc(sizeof(unsigned char)*num_image);

    for(int i = 0; i < num_image; i++){
        unsigned char label;
        // read the label from the file + error checking
        if(fread(&label, sizeof(unsigned char), 1, data_file)== 0){
            if(feof(data_file)){
                fprintf(stderr,"End of file\n");
            }else{
                perror("fread");
                exit(1);
            }
    }
        
        Image image;
        image.sx = WIDTH;
        image.sy = WIDTH;
        image.data = malloc(sizeof(unsigned char)*WIDTH*WIDTH);

        // read image data
        for( int j = 0; j < (WIDTH*WIDTH); j++){

            if(fread(&image.data[j], sizeof(unsigned char), 1, data_file)== 0){
                if(feof(data_file)){
                    fprintf(stderr,"End of file\n");
                }else{
                    perror("fread");
                    exit(1);
                    }
            }

            dataset->images[i] = image;
            dataset->labels[i] = label;
        }
    }
    int error = fclose(data_file);
    if(error!=0){
        fprintf(stderr, "Error: fclose data_file failed\n");
        exit(1);
    }
    return dataset;
    
}

/**
 * Compute and return the Gini impurity of M images at a given pixel
 * The M images to analyze are identified by the indices array. The M
 * elements of the indices array are indices into data.
 * This is the objective function that you will use to identify the best 
 * pixel on which to split the dataset when building the decision tree.
 *
 * Note that the gini_impurity implemented here can evaluate to NAN 
 * (Not A Number) and will return that value. Your implementation of the 
 * decision trees should ensure that a pixel whose gini_impurity evaluates 
 * to NAN is not used to split the data.  (see find_best_split)
 * 
 * DO NOT CHANGE THIS FUNCTION; It is already implemented for you.
 */
double gini_impurity(Dataset *data, int M, int *indices, int pixel) {
    int a_freq[10] = {0}, a_count = 0;
    int b_freq[10] = {0}, b_count = 0;

    for (int i = 0; i < M; i++) {
        int img_idx = indices[i];

        // The pixels are always either 0 or 255, but using < 128 for generality.
        if (data->images[img_idx].data[pixel] < 128) {
            a_freq[data->labels[img_idx]]++;
            a_count++;
        } else {
            b_freq[data->labels[img_idx]]++;
            b_count++;
        }
    }

    double a_gini = 0, b_gini = 0;
    for (int i = 0; i < 10; i++) {
        double a_i = ((double)a_freq[i]) / ((double)a_count);
        double b_i = ((double)b_freq[i]) / ((double)b_count);
        a_gini += a_i * (1 - a_i);
        b_gini += b_i * (1 - b_i);
    }

    // Weighted average of gini impurity of children
    return (a_gini * a_count + b_gini * b_count) / M;
}

/**
 * Given a subset of M images and the array of their corresponding indices, 
 * find and use the last two parameters (label and freq) to store the most
 * frequent label in the set and its frequency.
 *
 * - The most frequent label (between 0 and 9) will be stored in `*label`
 * - The frequency of this label within the subset will be stored in `*freq`
 * 
 * If multiple labels have the same maximal frequency, return the smallest one.
 */
void get_most_frequent(Dataset *data, int M, int *indices, int *label, int *freq) {
    // TODO: Set the correct values and return
    int count[10] = {0,0,0,0,0,0,0,0,0,0} ; //count of each label, from 0-9
    int indice;
    int cur_label;
    for (int i = 0; i < M; i++){    //update the count of labels
        indice = indices[i]; //which image we are analyzing
        cur_label = data ->labels[indice];
        count[cur_label] += 1;
    }
  
    int label_max_count = 0;    //the count of the most frequent label
    int max_label = 0;  //the most frequent label
    for (int j = 0; j < 10; j++){ //j is the label we are evaluating
        if(count[j] > label_max_count){
            label_max_count = count[j];
            max_label = j;
        }
    }
    *label = max_label;
    *freq = label_max_count;
}

/**
 * Given a subset of M images as defined by their indices, find and return
 * the best pixel to split the data. The best pixel is the one which
 * has the minimum Gini impurity as computed by `gini_impurity()` and 
 * is not NAN. (See handout for more information)
 * 
 * The return value will be a number between 0-783 (inclusive), representing
 *  the pixel the M images should be split based on.
 * 
 * If multiple pixels have the same minimal Gini impurity, return the smallest.
 */
int find_best_split(Dataset *data, int M, int *indices) {
    // TODO: Return the correct pixel
    double min_Gini = INFINITY;
    double cur_Gini ;
    int best_split = 0;
    for (int i = 0; i< 784; i++){  //i stands for the pixel
        cur_Gini = gini_impurity(data,  M, indices, i);

        if(cur_Gini < min_Gini){
            min_Gini = cur_Gini;
            best_split = i;
        }
    }
    return best_split;
}

/**
 * Create the Decision tree. In each recursive call, we consider the subset of the
 * dataset that correspond to the new node. To represent the subset, we pass 
 * an array of indices of these images in the subset of the dataset, along with 
 * its length M. Be careful to allocate this indices array for any recursive 
 * calls made, and free it when you no longer need the array. In this function,
 * you need to:
 *
 *    - Compute ratio of most frequent image in indices, do not split if the
 *      ration is greater than THRESHOLD_RATIO
 *    - Find the best pixel to split on using `find_best_split`
 *    - Split the data based on whether pixel is less than 128, allocate 
 *      arrays of indices of training images and populate them with the 
 *      subset of indices from M that correspond to which side of the split
 *      they are on
 *    - Allocate a new node, set the correct values and return
 *       - If it is a leaf node set `classification`, and both children = NULL.
 *       - Otherwise, set `pixel` and `left`/`right` nodes 
 *         (using build_subtree recursively). 
 */
DTNode *build_subtree(Dataset *data, int M, int *indices) {
    // TODO: Construct and return the tree
     if (M <= 0){
        return NULL;
    }
    DTNode *root;
    root = malloc(sizeof(DTNode));
    int label;
    int freq;
    get_most_frequent(data,  M, indices, &label, &freq); //store the most frequent label in label

   
    if(((float)freq)/M > THRESHOLD_RATIO){ //don't split and return root
        root->pixel = -1;
        root->classification = label;
        root->left = NULL;
        root->right = NULL;
        return root;
    }
    else { //need to split
        int split_indice;
        split_indice = find_best_split(data,  M, indices);
        int greater_num = 0;    //number of images whose split indice is greater than 128
        for (int i = 0; i < M; i++){
            if ((data->images[indices[i]]).data[split_indice] >= 128){
                greater_num++;
            }
        }
        int *indices_right = malloc(sizeof(int)*greater_num); //indices that go to the right subtree
        int *indices_left = malloc(sizeof(int)*(M-greater_num));
        int index_right = 0;
        int index_left = 0;
        for (int j = 0; j < M; j++){ //populate indices_right and indices_left
            
            if ((data->images[indices[j]]).data[split_indice] >= 128){
                indices_right[index_right] = indices[j];
                index_right++;
            }else{
                indices_left[index_left] = indices[j];
                index_left++;
            }
        }
       
      
        root->pixel = split_indice;
        root->classification = -1;
        
        root->right = build_subtree(data, greater_num, indices_right);
        
        root->left = build_subtree(data, M - greater_num, indices_left);
       
        free(indices_right);
        free(indices_left);
        return root;

       
    }
    
}

/**
 * This is the function exposed to the user. All you should do here is set
 * up the `indices` array correctly for the entire dataset and call 
 * `build_subtree()` with the correct parameters.
 */
DTNode *build_dec_tree(Dataset *data) {
    // TODO: Set up `indices` array, call `build_subtree` and return the tree.
    // HINT: Make sure you free any data that is not needed anymore
    int num_items = data->num_items;
    int *indices = malloc(sizeof(int)*num_items);
    for(int i = 0; i < num_items; i++){
        indices[i] = i;
    }
    DTNode *dtnode = build_subtree(data, num_items, indices);
    free(indices);
    return dtnode;
}

/**
 * Given a decision tree and an image to classify, return the predicted label.
 */
int dec_tree_classify(DTNode *root, Image *img) {
    // TODO: Return the correct label
    if(root == NULL){
        return -1;
    }
    int classification = root -> classification;
    if (classification != -1){
        return classification;
    }
    else{
        if((img->data)[root->pixel] >= 128){
            return dec_tree_classify(root->right, img);
        }
        else{
            return dec_tree_classify(root->left, img);
        }
    }

}



/**
 * This function frees the Decision tree.
 */
void free_dec_tree(DTNode *node) {
    // TODO: Free the decision tree
    DTNode *temp_left = node->left;
    DTNode *temp_right = node->right;
    free(node);
    if(temp_left != NULL){
        free_dec_tree(temp_left);
    }
    if(temp_right!=NULL){
        free_dec_tree(temp_right);
    }
    
     

}


/**
 * Free all the allocated memory for the dataset
 */
void free_dataset(Dataset *data) {
    // TODO: Free dataset (Same as A1)
    for(int i = 0; i < (data->num_items); i++){
    free((data->images[i]).data);
    }
    free(data->images);
    free(data->labels);
    free(data);

}


