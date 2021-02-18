//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package Project;

//
// TODO auto-generated module
//
simple Node
{
    parameters:
        double lambda = default(2.0);
        double delayRate = default(40);
		double dropRate = default(30);
		double duplicateRate = default(10);
		double corruptBitRate = default(100)  ;         
    gates:
        input ins[];
        output outs[];
}
