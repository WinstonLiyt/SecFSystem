## UNIX-like secondary file system  :yum:
### Experiment Name:
Constructing a UNIX-like secondary file system to implement basic file operations.


### Experiment Content:
Use a regular large file (e.g., c:\myDisk.img, referred to as the primary file) to simulate a UNIX V6++ disk. The information stored on the disk is organized in blocks, with each block being 512 bytes.


### Experiment Requirements:
1. Implement basic read and write operations on the logical disk.
2. Define the structure of a secondary file system on this logical disk, including:
    - **SuperBlock and Inode Area Location and Size**
    - **Inode Nodes**
        - Data Structure Definition: Pay attention to size, one disk block contains an integer number of Inode nodes.
        - Organization of the Inode Area: How to quickly locate a given Inode node number.
        - Index Structure: Composition of multi-level index structure, generation, and retrieval process of the index structure.
    - **SuperBlock**
        - Data Structure Definition
        - Design and implementation of algorithms for the allocation and recycling of Inode nodes.
        - Design and implementation of algorithms for the allocation and recycling of file data areas.
3. Directory Structure of the File System:
    - Structure of Directory Files
    - Design and implementation of directory retrieval algorithms
    - Design and implementation of directory structure addition, deletion, and modification
4. File Open Structure:
    - Design of File Open Structure: In-memory Inode node, File structure, Process open file table?
    - Allocation and recycling of in-memory Inode nodes
    - File open process
    - File close process
5. File Operation Interfaces:
    - **fformat**: Format the file volume
    - **ls**: Display all file names in the current path
    - **cd**: Change path (enter the next level or go back to the previous level)
    - **mkdir**: Create a directory
    - **fcreat**: Create a new file
    - **fopen**: Open a file
    - **fclose**: Close a file
    - **fread**: Read a file
    - **fwrite**: Write a file
    - **flseek**: Position the file read/write pointer
    - **fdelete**: Delete a file

  
### Experiment Environment:
- **Operating System**: Windows 10
- **Compiler**: Microsoft Visual Studio Community 2022
- **Plugin**: GitHub Copilot v1.177.0
