package com.xiaoboluo.d2r;
public final class MapGoalLayoutTest {
 static void check(boolean v,String s){if(!v)throw new AssertionError(s);}
 static float[] snapshot(){float[] s=new float[52];s[0]=1;s[1]=1;s[2]=4;s[3]=2000;s[4]=1000;s[5]=100;s[6]=200;s[7]=.5f;s[8]=.5f;s[9]=.7f;s[10]=.1f;s[11]=.98f;s[12]=.4f;s[16]=100;s[17]=200;s[18]=2;return s;}
 public static void main(String[] args){
  float[] s=snapshot();MapGoalLayout.Goal g=MapGoalLayout.project(s,0,2000,1000);
  check(g!=null && g.inside && g.x==1680 && g.y==250,"minimap center");
  s[16]=10000;g=MapGoalLayout.project(s,0,2000,1000);check(!g.inside && g.x<=1926.4f && g.y<=376,"offscreen clamped");
  for(float l:new float[]{.02f,.36f,.7f}){s=snapshot();s[9]=l;s[11]=l+.28f;g=MapGoalLayout.project(s,0,2000,1000);check(Math.abs(g.x-(l+.14f)*2000)<.1f,"left center right");}
  s=snapshot();s[0]=2;s[16]=110;s[17]=205;g=MapGoalLayout.project(s,0,2000,1000);check(g.inside && g.x==1032 && g.y==548,"original full map isometric projection");
  s[16]=10000;g=MapGoalLayout.project(s,0,2000,1000);check(!g.inside && g.y<=450,"fullmap skill region");
  s=snapshot();s[18]=70;g=MapGoalLayout.project(s,0,2000,1000);check(g.symbol && MapGoalLayout.labelName(g,1,false).equals("label_0.png"),"true tomb offscreen label");
  check(MapGoalLayout.labelName(g,1,true).equals("label_0_old.png"),"old simplified names");
  check(MapGoalLayout.labelName(g,5,false).equals("label_0_ko.png"),"Korean label");
  check(MapGoalLayout.markerName(g).equals("sym_6.png"),"inside true tomb uses original frame");
  check(MapGoalLayout.label(g,5,false,name->name.equals("label_0.png")?name:null).equals("label_0.png"),"missing localized art falls back to original Chinese");
  check(MapGoalLayout.label(g,5,false,name->null)==null,"missing art skipped without invented caption");
  for(int lang=0;lang<6;lang++)check(MapGoalLayout.labelName(g,lang,false).endsWith(new String[]{"_en.png","_0.png","_es.png","_mx.png","_ru.png","_ko.png"}[lang]),"language selection");
  s[0]=0;check(MapGoalLayout.project(s,0,2000,1000)==null,"closed");s[0]=1;s[1]=0;check(MapGoalLayout.project(s,0,2000,1000)==null,"no goals");
  s=snapshot();s[2]=Float.NaN;check(MapGoalLayout.project(s,0,2000,1000)==null,"bad geometry");
  s=snapshot();s[18]=99;check(MapGoalLayout.project(s,0,2000,1000)==null,"unknown host art hidden");
  s=snapshot();s[16]=Float.POSITIVE_INFINITY;check(MapGoalLayout.project(s,0,2000,1000)==null,"invalid goal hidden");
  MapGoalLayout.Box area=new MapGoalLayout.Box(0,0,1000,500);MapGoalLayout.Box block=new MapGoalLayout.Box(400,100,600,300);
  MapGoalLayout.Box moved=MapGoalLayout.place(500,200,100,30,area,new MapGoalLayout.Box[]{block});check(moved!=null && !moved.intersects(block),"actual obstruction avoidance");
  check(MapGoalLayout.place(500,200,100,30,area,new MapGoalLayout.Box[]{area})==null,"fully obstructed hidden");
  float[] controls=new float[235];controls[0]=1000;controls[1]=500;controls[4]=500;controls[5]=200;controls[6]=50;controls[7]=1;controls[185]=100;controls[186]=200;controls[187]=20;controls[188]=1;controls[230]=800;controls[231]=200;controls[232]=30;controls[233]=1;
  MapGoalLayout.Box[] obstacles=MapGoalLayout.controlBounds(controls,2000,1000);check(obstacles.length==3 && obstacles[0].left<1000 && obstacles[0].right>1000,"visible control ABI prefix and quick/cancel");
  controls[2]=1;check(!MapGoalLayout.gameplay(controls),"panel hidden");
  System.out.println("PASS original map projection, all positions, clamping, symbols, localization, hidden and obstruction layout");
 }
}
