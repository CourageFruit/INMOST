/*********************************
 Implementation of quaternion-based
 object rotation
 
 Functions
 clickmotion - call when holded mouse moved
 click       - call when user clicks
 motion      - call when mouse just moves
 quatinit    - flush rotation value
 rotate      - multiply GL matrix

 Dependency: rotate.h,
   Standard: math.h 
   Specific: glut.h
**********************************/

#ifndef _ROTATE_H
#define _ROTATE_H

void clickmotion(int nmx, int nmy);
void motion(int nmx, int nmy);
void click(int b, int s, int nmx, int nmy);
void quatinit();
void quatpush();
void quatpop();
void rotate();
void rotate_from_stack();
void rotatevector(double * vec);
void revrotatevector(double * vec);
void rotatevector_from_stack(double * vec);

#endif
