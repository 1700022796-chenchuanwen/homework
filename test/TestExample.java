import java.util.*;

class TestExample{

    public static void main(String[] args){
        double low=1, high =10;
        Scanner s = new Scanner(System.in);
        System.out.println("please input two numbers(interval with space):");
        low = s.nextInt();
        high = s.nextInt();

        if(low>high){
            System.out.println("error input");
            return;
        }

        //等价类划分
        double lowToHigh = high -low;
        double minToLow = low - Integer.MIN_VALUE ;
        double  highToMax = Integer.MAX_VALUE - high;

        ///边界值组
       List <Integer> boundaryLow = genBoundary((int)low); 
       List <Integer> boundaryHigh = genBoundary((int)high); 

      // System.out.print("boundary\n");
        //printCases(boundaryLow );
        //printCases(boundaryHigh );

        //平均每个范围取10组用例，如果不满十组，则尽可能取最大组数
        List<Integer> case1 = genRandomCases(minToLow, Integer.MIN_VALUE, low);
        List<Integer> case2 = genRandomCases(lowToHigh, low, high);
        List<Integer> case3 = genRandomCases(highToMax, high, Integer.MAX_VALUE);

       //System.out.print("random region:\n");
        //printCases(case1);
        //printCases(case2);
        //printCases(case3);

        List<Integer> allList = new ArrayList<Integer>();

        //去重
        boundaryLow.removeAll(boundaryHigh);
        case2.removeAll(boundaryLow);
        case2.removeAll(boundaryHigh);
        //合并
        allList.addAll(boundaryLow);
        allList.addAll(boundaryHigh);
        allList.addAll(case1);
        allList.addAll(case2);
        allList.addAll(case3);
       
        //排序 
        Collections.sort(allList);
        printCases(allList); 
    }

    public static List<Integer> genBoundary(int e){
        List<Integer> arr = new ArrayList<Integer>();

        if(e> Integer.MIN_VALUE +1 && e< Integer.MAX_VALUE){
            arr.add(Integer.MIN_VALUE);
            arr.add(e-1);
            arr.add(e);
            arr.add(e+1);
            arr.add(Integer.MAX_VALUE);
        }else if(e == Integer.MIN_VALUE+1){
            arr.add(Integer.MIN_VALUE);
            arr.add(Integer.MIN_VALUE-1);
            arr.add(Integer.MAX_VALUE);
        }else if(e == Integer.MIN_VALUE){
            arr.add(Integer.MIN_VALUE);
            arr.add(Integer.MAX_VALUE);
        }else if(e == Integer.MAX_VALUE-1){
            arr.add(Integer.MIN_VALUE);
            arr.add(Integer.MAX_VALUE-1);
            arr.add(Integer.MAX_VALUE);
        }else{
            arr.add(Integer.MIN_VALUE);
            arr.add(Integer.MAX_VALUE);
        }
        return arr;
    }

    //层次划分 + 随机数
    public static List<Integer> genRandomCases(double length, double low, double high){
        List<Integer> list = new ArrayList<Integer>();
        double step=0;
        int len;
        if(length >10){
            step = length/10;
            len=10;
        }else{
            step =1; 
            len = (int)length;
        }

        int[] arr = new int[len];
        double tmpLow,tmpHigh;
        int tmp;

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

        return list;
    }
    public static void printCases(List<Integer> list){
        for(int i=0;i<list.size();i++){
            System.out.print(list.get(i) +" ");
        }
        System.out.print("\n");
    }

}
