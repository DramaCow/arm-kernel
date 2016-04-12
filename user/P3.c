#include "P3.h"

void P3() {
  const int FILE = fopen( "/file.txt" );

  //char *text = "Heterodontosaurus is a genus of heterodontosaurid dinosaur that lived during the Early Jurassic period, 200–190 million years ago. Its only known member species, Heterodontosaurus tucki, was named in 1962 based on a skull discovered in South Africa. The genus name means \"different toothed lizard\", in reference to its unusual, heterodont dentition; the specific name honors G. C. Tuck, who supported the discoverers. Further specimens have since been found, including an almost complete skeleton in 1966.\n\nThough it was a small dinosaur, Heterodontosaurus was one of the largest members of its family, reaching between 1.18 m (3.9 ft) and possibly 1.75 m (5.7 ft) in length, and weighing between 2 and 10 kg (4.4 and 22.0 lb). The body was short with a long tail. The five-fingered forelimbs were long and relatively robust, whereas the hind-limbs were long, slender, and had four toes. The skull was elongated, narrow, and triangular when viewed from the side. The front of the jaws were covered in a horny beak. It had three types of teeth; in the upper jaw, small, incisor-like teeth were followed by long, canine-like tusks. A gap divided the tusks from the chisel-like \"cheek teeth\".\n\nHeterodontosaurus is the eponymous and best known member of the family Heterodontosauridae. This family is considered one of the most primitive or basal groups within the order of ornithischian dinosaurs. In spite of the large tusks, Heterodontosaurus is thought to have been herbivorous, or at least omnivorous. Though it was formerly thought to have been capable of quadrupedal locomotion, it is now thought to have been bipedal. Tooth replacement was sporadic and not continuous, unlike its relatives. At least four other heterodontosaurid genera are known from the same geological formations as Heterodontosaurus.";

  //write( FILE, text, 1810 );

  char buf[ 64 ];
  
  write( FILE, "message message", 15 );
  fseek( FILE, 0, SEEK_SET );
  write( FILE, "cabb", 4 );
  fseek( FILE, -4, SEEK_CUR );  

  read( FILE, buf, 15 );
  write( STDIO, buf, 15 ); 

  fclose( FILE );

/*  int m = mqinit(100);*/

/*  int message = 15061996;*/
/*  msgsend( m, &message, sizeof( int ) );*/

/*  msgreceive( m, &message, sizeof( int ) );*/
/*  if (message == 2) write( STDIO, "success2", 8 );*/

/*  message = 3;*/
/*  msgsend( m, &message, sizeof( int ) );*/

/*  msgreceive( m, &message, sizeof( int ) );*/
/*  if (message == 4) write( STDIO, "success4", 8 );*/

  cexit();
}

void (*entry_P3)() = &P3;
