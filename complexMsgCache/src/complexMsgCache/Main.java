package complexMsgCache;

import java.util.Random;

public class Main {
	public static void main(String[] args) {
		
		
		Producer p[] = new Producer [Global.m]; //m个生产者
		Consumer c[]  = new Consumer [Global.n];  //n个消费者
		int i;
		for(i=0;i<Global.m;i++) {
			p[i] = new Producer(i+1);
		}

		for(i=0;i<Global.n;i++) {
			c[i] = new Consumer(i+1);
		}
		
		//初始化线程
		Thread pt[] = new Thread[Global.m];
		Thread ct[] = new Thread[Global.n];
		
		for(i=0;i<Global.m;i++) {
			pt[i] = new Thread(p[i]);
		}
		
		for(i=0;i<Global.n;i++) {
			ct[i] = new Thread(c[i]);
		}
		
		for(i=0;i<Global.m;i++) {
			pt[i].start();
		}
		
		for(i=0;i<Global.n;i++) {
			ct[i].start();
		}
				
	
	}
	
}

class Global{
	
	static int k=8;
	static int m=3;
	static int n=5;
	static int msgCnt =20;
	
	static Syn empty = new Syn(k);
	static Syn full = new Syn(0);
	static int buffer[] = new int[k];
	static boolean readable[] = new boolean[k];
	static boolean record[][] = new boolean [k][n];  //消费者取走标记
	
	static Syn pMutex = new Syn(1);  //生产者互斥
	static Syn cMutex = new Syn(1);  //消费者互斥
	static Syn Mutex =  new Syn(1);  //数据区读写互斥
	
	static int pCount=0;
	static int cCount[]= new int[n];
	
	
}

//生产者
class Producer implements  Runnable{
	int ID=0;
	Producer(){};
	Producer(int id){
		ID = id;
	};

	public void run() {
		while(true) {
			Global.pMutex.Wait();//生产者互斥
			Global.empty.Wait(); //等待缓冲区空余
			//临界区start	
			if(Global.pCount >=Global.msgCnt) {
				//如果已经生产足够的消息，则退出。
				//临界区end
				Global.pMutex.Signal();
				Global.full.Signal();
				break;
			}

			//读写互斥
			Global.Mutex.Wait();
			//生产消息
			for(int i=0;i<Global.k;i++) {
				int index=(Global.pCount+i)%Global.k;

				if(Global.readable[index] == false){
					Global.buffer[index] = Global.pCount;
					System.out.println(">>生产者"+ID+"在缓冲区"+index+"中生产了消息"+Global.pCount);
					Global.pCount++; //生产消息数+1
					
					Global.readable[index] = true; //可读
					
					//初始化该缓冲区的被各消费者取走的状态为0
					for(int j=0;j<Global.n;j++) {
						Global.record[index][j]= false;
					}
					Global.full.Signal();  //通知消费者
					break;
				}
			}
			//临界区end
			Global.Mutex.Signal(); //释放数据区
		
			Global.pMutex.Signal();  //释放互斥锁			

			try {
				Thread.sleep(10);
			} catch(InterruptedException e) {
				e.printStackTrace();
			}

					
		}
	}
}

//消费者
class Consumer implements Runnable{
	int ID=0;
	int curIndex = (int)(Math.random()*10);
	int msg[] = new int[Global.msgCnt];
	Consumer(){};
	Consumer(int id){
		ID = id;
	};
	public void run() {
		while(true) {
			Global.cMutex.Wait();  //消费者互斥锁
			Global.full.Wait();    //等待缓冲区有数据
			//临界区start
			if(Global.cCount[ID-1] >=Global.msgCnt) {
				//如果已经取了消息数*消费者数上限，则退出临界区
				//临界区end
				Global.empty.Signal();
				Global.full.Signal();  //通知其他消费者
				Global.cMutex.Signal();
				System.out.println("消费者取消息达到上限！！！");
				break;
			}
			
			//读写互斥
			Global.Mutex.Wait();
			
			int i, iIndex=curIndex;
			for(i=0;i<Global.k;i++) {
				iIndex =  (curIndex +i+1 )%Global.k;
				//检查是否已经取走了缓冲区的消息
				if(Global.readable[iIndex] && Global.record[iIndex][ID-1] == false) {
					//取走消息标记
					Global.record[iIndex][ID-1] =true;
					Global.cCount[ID-1]++;
					System.out.println("消费者"+ID+"在缓冲区"+iIndex+"中取走了消息"+Global.buffer[iIndex]);
					break;
				}
			}
			
			curIndex = iIndex; //下一轮 从下一个缓冲区取，避免饥饿状态
				
			boolean isLast = true;
			//判断是否最后一个读消息的消费者
			for(int j=0;j<Global.n;j++) {
				//如果还有其他消费者未取，则不是最后一个
			   if(Global.record[iIndex][j] == false) {
				   isLast = false;
				   break;
			   }
			}
			
			//临界区 end
			if(isLast) {
				Global.readable[iIndex]= false;
				System.out.println("（缓冲区"+iIndex+"消息"+Global.buffer[iIndex]+"已经被取空）");
				Global.empty.Signal();
			}else {
				Global.full.Signal();  //通知其他消费者
			}
			
			Global.Mutex.Signal(); //释放数据区

			Global.cMutex.Signal();
			
			

			try {
				Thread.sleep(10);
			}catch(InterruptedException e) {
				e.printStackTrace();
			}

		}
	}
}


