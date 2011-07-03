int size, ch, ii;

while (1) {
    if (signal()) {
        ch = input();
        switch (ch) {
            case 'I':
                vcap();
                size = vjpeg(4);
                printf("##IMJ5%c%c%c%c", size & 0x000000FF, (size & 0x0000FF00) / 0x100, (size & 0x00FF0000) / 0x10000, 0);
                vsend(size);
                break;
            case '@':   // sync for data input
                printf("#@");
                ii = read_int();
                printf("read_int() returned %d\r\n", ii);
                break;
            case 0x1b:  // ESC
                exit();
        }
    }
}

