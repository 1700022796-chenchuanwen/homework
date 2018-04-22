import java.util.*;

class TestExample{

    public static void main(String[] args){
        double low=1, high =10;
        Scanner s = new Scanner(System.in);
   
        //输入校验 
        boolean checkFlag = false;
        while(checkFlag ==false){
            System.out.println("please input two numbers(interval with space):");
            try{
                low = s.nextInt();
                high = s.nextInt();
            }catch(Exception e){
                System.out.println("input error:Please input Integer numbers!(tips:"+ Integer.MIN_VALUE +"~" + Integer.MAX_VALUE+")");
                s.nextLine();
                checkFlag = false;
                continue;
            }

            if(low>high){
                System.out.println("error input: first number should be smaller than the second");
                checkFlag = false;
            }else{
                checkFlag = true;
            }
        }
        //--------------------------------------------------------------------------
        //等价类划分
        ArrayList<ArrayList<Integer>>partition = genEqualencePartition(low, high);

        //边界值
        ArrayList<ArrayList<Integer>> boundary =  genBoundary(partition);

        //随机数
        //平均每个范围取10组用例，如果不满十组，则尽可能取最大组数
        ArrayList<Integer> cases = genRandomCases(boundary); 

        //---------------------------------------------------------------------
        printCases(cases); 
    }

    public static  ArrayList<ArrayList<Integer>>genEqualencePartition(double low, double high){
       ArrayList<ArrayList<Integer>>  partList = new ArrayList<ArrayList<Integer>>();
       ArrayList pList = new ArrayList<Double>();


        pList.add(Integer.MIN_VALUE);
        pList.add((int)low);
        partList.add(pList);
        //--------------
        pList = new ArrayList<Double>();
        pList.add((int)low);
        pList.add((int)high);
        partList.add(pList);
        //--------------------
        pList = new ArrayList<Double>();
        pList.add((int)high);
        pList.add(Integer.MAX_VALUE);
        partList.add(pList);
    
        return partList; 
    }

    public static ArrayList<ArrayList<Integer>> genBoundary(ArrayList<ArrayList<Integer>>part){
       ArrayList<ArrayList<Integer>>  boundary = new ArrayList<ArrayList<Integer>>();
       int size = part.size(); 
       for(int i=0; i<size;i++){
            int len = part.get(i).size();
            for(int j=0;j<len;j++){
                ArrayList bound = new ArrayList<Double>();
                int e ;
            
                if(i!=size-1 && j!=0){
                    continue;
                }

                e =  part.get(i).get(j);

                if(e> Integer.MIN_VALUE +1 && e< Integer.MAX_VALUE){
                    if(bound.indexOf(e-1) ==-1)  bound.add(e-1);
                    if(bound.indexOf(e) ==-1 ) bound.add(e);
                    if(bound.indexOf(e+1) ==-1 ) bound.add(e+1);
                }else if(e == Integer.MIN_VALUE+1){
                    if(bound.indexOf(Integer.MIN_VALUE) ==-1 ) bound.add(Integer.MIN_VALUE);
                    if(bound.indexOf(Integer.MIN_VALUE-1) ==-1 ) bound.add(Integer.MIN_VALUE-1);
                }else if(e == Integer.MIN_VALUE){
                    if(bound.indexOf(Integer.MIN_VALUE) ==-1 ) bound.add(Integer.MIN_VALUE);
                    if(bound.indexOf(Integer.MAX_VALUE+1) ==-1 ) bound.add(Integer.MAX_VALUE+1);
                }else if(e == Integer.MAX_VALUE-1){
                    if(bound.indexOf(Integer.MAX_VALUE-1) ==-1 ) bound.add(Integer.MAX_VALUE-1);
                    if(bound.indexOf(Integer.MAX_VALUE) ==-1 ) bound.add(Integer.MAX_VALUE);
                }else if(e == Integer.MAX_VALUE){
                    if(bound.indexOf(Integer.MAX_VALUE) ==-1 ) bound.add(Integer.MAX_VALUE);
                }
                boundary.add(bound);
            }
        

       }

        return boundary; 

    }    

    public static ArrayList<Integer> genRandomCases( ArrayList<ArrayList<Integer>> boundary){
        ArrayList<Integer>rsList = new ArrayList<Integer>();

        double step=0;
        int len,len1;
        double tmpLow,tmpHigh;
        int tmp;
        ArrayList<Integer> tmpList;
        
        int size = boundary.size(); //查询有多少组边界
           
        for(int k =0;k<size-1;k++ ){
            ArrayList<Integer> list = new ArrayList<Integer>();

            tmpList= boundary.get(k); 
            tmpLow = tmpList.get(tmpList.size()-1); 
            //rsList.add(tmpList);
            len1 = tmpList.size();
            for(int i1=0;i1<len1;i1++){
                int tmp1 = tmpList.get(i1);
                if(rsList.indexOf(tmp1) ==-1){
                    rsList.add(tmp1);
                }
            }

            tmpList= boundary.get(k+1);
            tmpHigh = tmpList.get(0); 

            double low = tmpLow;
            double high= tmpHigh;

            double length = tmpHigh -tmpLow;

            if(length >10){
                step = length/10;
                len=10;
            }else if(length>0){
                step =1; 
                len = (int)length;
            }else{
                len =0;
            }

            int[] arr = new int[len];
            for(int i=0;i<len;i++){
                tmpLow = low + (i*step);
                tmpHigh = low + ((i+1)*step);

                // System.out.println(tmpLow+" -" +tmpHigh+":"+step+":len:"+len);
                tmp = (int) (Math.random() * (tmpHigh-tmpLow) + tmpLow);
                if(tmp ==low || tmp==high){
                    continue;
                }else if(list.indexOf(tmp) ==-1){
                    list.add(tmp);
                }else{
                    list.add(tmp+1);
                }
            }
            
            len1 = list.size();
            for(int i1=0;i1<len1;i1++){
                int tmp1 = list.get(i1);
                if(rsList.indexOf(tmp1) ==-1){
                     rsList.add(tmp1);
                }
            }
            //rsList.add(list); 
      
        }
        tmpList = boundary.get(size-1);
        len1 = tmpList.size();

        for(int i1=0;i1<len1;i1++){
            int tmp1 = tmpList.get(i1);
            if(rsList.indexOf(tmp1) ==-1){
                rsList.add(tmp1);
            }
        }
        //rsList.add(tmpList); 

        return rsList;
    }

    public static void printCases(ArrayList<Integer> list){
        System.out.print("\n");
        for(int i =0;i<list.size();i++ ){
            System.out.print(list.get(i) +" ");
        }
    }

}
